#include "../include/progressive/incremental.h"
#include "../include/util/binary_search.h"
#include "../include/full_index/hybrid_radix_insert_sort.h"
#include "../include/progressive/constants.h"
#include <algorithm>
#include <string.h>
#include <memory>

using namespace std;


#define RADIXSORT_MSD_BYTES 8
// total amount of relevant bytes in the data set
// this is ceil(log2(MAX_VALUE)) of the data set
// since our max value is 10^8, ceil(log2(10^8)) = 27
extern int RADIXSORT_TOTAL_BYTES;

constexpr size_t RADIX_BUCKET_COUNT = 1 << RADIXSORT_MSD_BYTES;

static inline bucket_type get_bucket_index(int64_t point, int64_t mask, int64_t shift) {
    return (point & mask) >> shift;
}

ResultStruct range_query_incremental_radixsort_msd_noquick(Column &c, int64_t low, int64_t high, double delta) {
    ResultStruct results;
    results.reserve(c.data.size());

    // first search the already indexed buckets for elements
    if (c.bucket_index.buckets.size() == 0) {
        c.bucket_index.buckets.resize(RADIX_BUCKET_COUNT);
        // create all bitshifts and bitmasks
        int64_t current_shift = RADIXSORT_TOTAL_BYTES - RADIXSORT_MSD_BYTES;
        c.msd.shifts.push_back(current_shift);
        c.msd.masks.push_back((RADIX_BUCKET_COUNT - 1) << current_shift);
        current_shift -= RADIXSORT_MSD_BYTES;
        c.msd.shifts.push_back(current_shift);
        c.msd.masks.push_back((RADIX_BUCKET_COUNT - 1) << current_shift);

        c.msd.remaining_buckets = 1 << current_shift;
        current_shift -= RADIXSORT_MSD_BYTES;
        c.msd.shifts.push_back(current_shift);
        c.msd.masks.push_back((c.msd.remaining_buckets - 1) << current_shift);
    }

    if (!c.msd.prev_array) {
        auto first_bucket_id = low >> c.msd.shifts[0];
        auto last_bucket_id = high >> c.msd.shifts[0];
        for (size_t i = first_bucket_id; i <= last_bucket_id; i++) {
            BucketRoot &bucket = c.bucket_index.buckets[i];
            if (i > first_bucket_id && i < last_bucket_id) {
                bucket.AddAllElements(results);
            } else {
                bucket.RangeQuery(low, high, results);
            }
        }
    } else {
        int64_t start_pos = 0, end_pos = 0;
        // get the bucket position for the start and end entry
        for(size_t i = 0; i < c.msd.shift_index; i++) {
            start_pos = start_pos * RADIX_BUCKET_COUNT + get_bucket_index(low, c.msd.masks[i], c.msd.shifts[i]);
            end_pos = end_pos * RADIX_BUCKET_COUNT + get_bucket_index(high, c.msd.masks[i], c.msd.shifts[i]);
        }
        // now perform the actual scan
        // the results MAYBE match in the buckets that "start_pos" and "end_pos" fall into
        // first check the start_pos bucket
        if (start_pos == end_pos) {
            // both are in the same bucket, just scan this bucket
            size_t start = c.msd.prev_offsets[start_pos];
            size_t end = end_pos + 1 == c.msd.prev_bucket_count ? c.data.size() : c.msd.prev_offsets[end_pos + 1];
            for(size_t i = start; i < end; i++) {
                int matching = c.msd.prev_array[i] >= low && c.msd.prev_array[i] <= high;
                results.maybe_push_back(c.msd.prev_array[i], matching);
            }
        } else {
            // both are different buckets, scan the "start" and "end" buckets separately
            size_t first_bucket_start = c.msd.prev_offsets[start_pos];
            size_t first_bucket_end = c.msd.prev_offsets[start_pos + 1];
            size_t last_bucket_start = c.msd.prev_offsets[end_pos];
            size_t last_bucket_end = end_pos + 1 == c.msd.prev_bucket_count ? c.data.size() : c.msd.prev_offsets[end_pos + 1];
            // first scan the start bucket
            for(size_t i = first_bucket_start; i < first_bucket_end; i++) {
                int matching = c.msd.prev_array[i] >= low && c.msd.prev_array[i] <= high;
                results.maybe_push_back(c.msd.prev_array[i], matching);
            }
            // scan everything in between, for this we know that the query predicate always matches
            for(size_t i = first_bucket_end; i < last_bucket_start; i++) {
                results.push_back(c.msd.prev_array[i]);
            }
            // finally scan the end bucket
            for(size_t i = last_bucket_start; i < last_bucket_end; i++) {
                int matching = c.msd.prev_array[i] >= low && c.msd.prev_array[i] <= high;
                results.maybe_push_back(c.msd.prev_array[i], matching);
            }
        }
    }

    if (c.bucket_index.index_position < c.data.size()) {
        // initial run: have to insert elements into the buckets
        size_t next_position = std::min(c.bucket_index.index_position + (size_t) (c.data.size() * delta),
                                        c.data.size());
        for (auto &i = c.bucket_index.index_position; i < next_position; i++) {
            auto bucket_id = c.data[i] >> c.msd.shifts[0];
            c.bucket_index.buckets[bucket_id].AddElement(i, c.data[i]);
            int matching = c.data[i] >= low && c.data[i] <= high;
            results.maybe_push_back(c.data[i], matching);
        }
        // now scan the remainder of the data
        for (size_t i = next_position; i < c.data.size(); i++) {
            int matching = c.data[i] >= low &&
                           c.data[i] <= high;
            results.maybe_push_back(c.data[i], matching);
        }
    } else if (c.msd.prev_array_index == -1) {
        int64_t remaining_budget = c.data.size() * delta;
        // subsequent run: move elements from the previous iteration of buckets to the next iteration of buckets
        if (c.msd.current_bucket_count == 0) {
            // current shift is the left shift of the next set of bytes
            // mask is the bitmask that masks out just the next set of bytes
            c.msd.current_bucket = 0;
            c.msd.current_bucket_count = RADIX_BUCKET_COUNT * RADIX_BUCKET_COUNT;
            c.msd.data = unique_ptr<int64_t[]>(new int64_t[c.data.size()]);
            c.msd.offsets = unique_ptr<int64_t[]>(new int64_t[c.msd.current_bucket_count]);
            c.msd.counts = unique_ptr<int64_t[]>(new int64_t[RADIX_BUCKET_COUNT]);
            memset(c.msd.counts.get(), 0, sizeof(int64_t) * RADIX_BUCKET_COUNT);
            c.msd.current_entry = c.bucket_index.buckets[0].head;
        }
        int64_t current_shift = c.msd.shifts[1];
        int64_t current_mask = c.msd.masks[1];
        while(remaining_budget > 0) {
            auto &bucket = c.bucket_index.buckets[c.msd.current_bucket];
            if (bucket.head) {
                if (!c.msd.found_boundaries) {
                    // first we need to find the boundaries for this bucket
                    // loop over all the buckets
                    while(remaining_budget > 0) {
                        remaining_budget -= c.msd.current_entry->size;
                        for(size_t i = 0; i < c.msd.current_entry->size; i++) {
                            // for every element in this bucket, figure out the bucket it belongs to and increment the offset
                            auto bucket_index = get_bucket_index(c.msd.current_entry->data[i], current_mask, current_shift);
                            c.msd.counts[bucket_index]++;
                        }
                        // done with this bucket, go to the next one
                        c.msd.current_entry = c.msd.current_entry->next;
                        if (!c.msd.current_entry) {
                            // done with this bucket
                            c.msd.found_boundaries = true;
                            c.msd.current_entry = c.bucket_index.buckets[c.msd.current_bucket].head;
                            // insert the counts into the offsets array
                            int64_t current_offset = c.msd.array_offset;
                            for(size_t i = 0; i < RADIX_BUCKET_COUNT; i++) {
                                c.msd.offsets[c.msd.initial_offset + i] = current_offset;
                                current_offset += c.msd.counts[i];
                                c.msd.counts[i] = 0;
                            }
                            break;
                        }
                    }
                    continue;
                } else {
                    // now that we have found the boundaries, actually perform the splitting into the different locations
                    bool done = false;
                    while(remaining_budget > 0) {
                        remaining_budget -= c.msd.current_entry->size;
                        for(size_t i = 0; i < c.msd.current_entry->size; i++) {
                            // for every element in this bucket, figure out the bucket it belongs to and write the element there
                            auto point = c.msd.current_entry->data[i];
                            auto bucket_index = get_bucket_index(point, current_mask, current_shift);
                            c.msd.data[c.msd.offsets[c.msd.initial_offset + bucket_index] + c.msd.counts[bucket_index]] = point;
                            c.msd.counts[bucket_index]++;
                        }
                        // done with this bucket, go to the next one
                        c.msd.current_entry = c.msd.current_entry->next;
                        if (!c.msd.current_entry) {
                            // done with this bucket entirely
                            done = true;
                            break;
                        }
                    }
                    if (!done) {
                        break;
                    }
                }
            } else {
                int64_t current_offset = c.msd.array_offset;
                for(size_t i = 0; i < RADIX_BUCKET_COUNT; i++) {
                    c.msd.offsets[c.msd.initial_offset + i] = current_offset;
                }
            }
            // move on to the next bucket
            c.msd.initial_offset += RADIX_BUCKET_COUNT;
            c.msd.array_offset += bucket.count;
            c.msd.current_bucket++;
            c.msd.found_boundaries = false;
            memset(c.msd.counts.get(), 0, sizeof(int64_t) * RADIX_BUCKET_COUNT);
            if (c.msd.current_bucket < RADIX_BUCKET_COUNT) {
                c.msd.offsets[c.msd.initial_offset] = c.msd.array_offset;
                c.msd.current_entry = c.bucket_index.buckets[c.msd.current_bucket].head;
            } else {
                // done with all buckets!
                c.msd.found_boundaries = false;
                c.msd.prev_bucket_count = c.msd.current_bucket_count;
                c.msd.prev_offsets = move(c.msd.offsets);
                c.msd.prev_array = move(c.msd.data);
                c.msd.prev_array_index = 0;
                c.msd.shift_index = 2;
                c.msd.current_bucket_count *= c.msd.remaining_buckets;
                c.msd.array_offset = 0;
                c.msd.data = unique_ptr<int64_t[]>(new int64_t[c.data.size()]);
                c.msd.offsets = unique_ptr<int64_t[]>(new int64_t[c.msd.remaining_buckets]);
                c.msd.counts = unique_ptr<int64_t[]>(new int64_t[c.msd.remaining_buckets]);
                memset(c.msd.counts.get(), 0, sizeof(int64_t) * c.msd.remaining_buckets);
                break;
            }
        }
    } else {
        int64_t remaining_budget = c.data.size() * delta;
        // final runs: move elements into the result array
        int64_t current_shift = c.msd.shifts[c.msd.shift_index];
        int64_t current_mask = c.msd.masks[c.msd.shift_index];
        while(remaining_budget > 0) {
            int64_t start = c.msd.array_offset;
            int64_t end = c.msd.prev_array_index + 1 == c.msd.prev_bucket_count ? c.data.size() : c.msd.prev_offsets[c.msd.prev_array_index + 1];
            int64_t len = end - start;
            if (len < 1024) {
                // small bucket: insert into final array and sort
                remaining_budget -= len * 5;
                memcpy(c.msd.data.get() + start, c.msd.prev_array.get() + start, len * sizeof(int64_t));
                std::sort(c.msd.data.get() + start, c.msd.data.get() + start + len);
            } else {
                //bigger bucket: have to perform more partitioning
                //first find the boundaries
                if (!c.msd.found_boundaries) {
                    // first we need to find the boundaries for this bucket, loop over all the entries that we can given our budget
                    int64_t target = std::min(remaining_budget, end - c.msd.array_offset_bucket);
                    remaining_budget -= target;
                    int64_t current_end = c.msd.array_offset_bucket + target;
                    for(int64_t i = c.msd.array_offset_bucket; i < current_end; i++) {
                        auto bucket_index = get_bucket_index(c.msd.prev_array[i], current_mask, current_shift);
                        c.msd.counts[bucket_index]++;
                    }
                    c.msd.array_offset_bucket = current_end;
                    if (c.msd.array_offset_bucket == end) {
                        // finished finding the boundaries
                        c.msd.found_boundaries = true;
                        c.msd.array_offset_bucket = c.msd.array_offset;
                        // generate the offsets
                        int64_t current_offset = c.msd.array_offset;
                        for(size_t i = 0; i < c.msd.remaining_buckets; i++) {
                            c.msd.offsets[i] = current_offset;
                            current_offset += c.msd.counts[i];
                            c.msd.counts[i] = 0;
                        }
                    }
                    continue;
                } else {
                    // now that we have found the boundaries, actually perform the splitting into the different locations
                    int64_t target = std::min(remaining_budget, end - c.msd.array_offset_bucket);
                    remaining_budget -= target;
                    int64_t current_end = c.msd.array_offset_bucket + target;
                    c.msd.array_offset_bucket = current_end;
                    for(int64_t i = c.msd.array_offset_bucket; i < current_end; i++) {
                        auto point = c.msd.prev_array[i];
                        auto bucket_index = get_bucket_index(point, current_mask, current_shift);
                        c.msd.data[c.msd.offsets[bucket_index] + c.msd.counts[bucket_index]] = point;
                        c.msd.counts[bucket_index]++;
                    }
                    if (c.msd.array_offset_bucket == end) {
                        // done! reset count array
                        memset(c.msd.counts.get(), 0, sizeof(int64_t) * c.msd.remaining_buckets);
                    } else {
                        break;
                    }
                }
            }
            // move on to the next bucket
            c.msd.prev_array_index++;
            c.msd.array_offset = end;
            c.msd.array_offset_bucket = end;
            c.msd.found_boundaries = false;
            if (c.msd.prev_array_index + 1 == c.msd.prev_bucket_count) {
                c.converged = true;
                c.final_data = c.msd.data.get();
                break;
            }

        }
    }
    return results;
}


double get_estimated_time_radixsort_msd_noquick(Column &c, int64_t low, int64_t high, double delta) {
    return 0;
}

void IncrementalMSDIndex::clear() {
    prev_array_index = -1;
    found_boundaries = false;
    current_bucket = 0;
    current_bucket_count = 0;
    initial_offset = 0;
    prev_bucket_count = 0;
    current_entry = nullptr;
    counts = nullptr;
    offsets = nullptr;
    data = nullptr;
    prev_offsets = nullptr;
    prev_array = nullptr;
    array_offset = 0;
}

void IncrementalMSDIndex::Copy(Column& c, IncrementalMSDIndex& target) {
    throw 5;
}