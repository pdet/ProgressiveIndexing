#include "../include/progressive/incremental.h"
#include "../include/util/binary_search.h"
#include "../include/full_index/hybrid_radix_insert_sort.h"
#include "../include/progressive/constants.h"

using namespace std;


#define EQUIHEIGHT_BUCKET_COUNT 128
#define EQUIHEIGHT_MID_POINT 64
#define EQUIHEIGHT_75 96
#define EQUIHEIGHT_25 32


static inline int GetBucketIDEquiHeight(std::vector<int64_t>& bounds, int64_t point) {
    int bucket = 0;
    if (point > bounds[EQUIHEIGHT_MID_POINT]) {
        if (point > bounds[EQUIHEIGHT_75]) {
            for(size_t i = EQUIHEIGHT_75; i < bounds.size(); i++) {
                bucket += point > bounds[i];
            }
            return bucket + EQUIHEIGHT_75;
        } else {
            for(size_t i = EQUIHEIGHT_MID_POINT; i < EQUIHEIGHT_75; i++) {
                bucket += point > bounds[i];
            }
            return bucket + EQUIHEIGHT_MID_POINT;
        }
    } else {
        if (point > bounds[EQUIHEIGHT_25]) {
            for(size_t i = EQUIHEIGHT_25; i < EQUIHEIGHT_MID_POINT; i++) {
                bucket += point > bounds[i];
            }
            return bucket + EQUIHEIGHT_25;
        } else {
            for(size_t i = 0; i < EQUIHEIGHT_25; i++) {
                bucket += point > bounds[i];
            }
            return bucket;
        }
    }
}


ResultStruct range_query_incremental_bucketsort_equiheight(Column& c, int64_t low, int64_t high, double delta){
    ResultStruct results;
    results.reserve(c.data.size());

    // assert(c.sortindex.size() == c.bucket_index.final_index.size());
    // for(size_t i = 0; i < c.sortindex.size(); i++) {
    // 	assert(c.data[c.sortindex[i]] == c.bucket_index.final_index[i]);
    // }

    if (c.bucket_index.final_index_entries == c.data.size()) {
        c.converged = true;
        c.final_data = c.bucket_index.final_index;
        range_query_sorted_subsequent_value(c.bucket_index.final_index, c.data.size(), low, high, results);
        return results;
    }

    if (c.bucket_index.max_value == INT_MIN) {
        // first run: sample from the data to obtain a histogram
        std::vector<int64_t> samples;
        samples.reserve(1 + c.data.size() / 8192);

        assert(c.data.size() / 8192 >= EQUIHEIGHT_BUCKET_COUNT);

        size_t i = 0;
        for(size_t j = 0; j <= c.data.size() / 8192; j++) {
            size_t next = std::min(c.data.size(), i + 8192);
            samples.push_back(c.data[i]);
            for(; i < next; i++) {
                int matching = c.data[i] >= low && c.data[i] <= high;
				results.maybe_push_back(c.data[i], matching);
            }
        }
        std::sort(samples.begin(), samples.end());

        size_t bucket_count = 0;
        c.bucket_index.buckets.resize(1 + EQUIHEIGHT_BUCKET_COUNT);
        for(size_t i = 0; i < samples.size(); i += samples.size() / EQUIHEIGHT_BUCKET_COUNT) {
            if (i > 0 && samples[i - 1] == samples[i]) continue;

            c.bucket_index.bounds.push_back(samples[i]);
            c.bucket_index.buckets[bucket_count].head = new BucketEntry();
            c.bucket_index.buckets[bucket_count].tail = c.bucket_index.buckets[bucket_count].head;
            c.bucket_index.buckets[bucket_count].head->size = 0;
            bucket_count++;
            if (bucket_count == c.bucket_index.buckets.size() - 1) {
                break;
            }
        }
        c.bucket_index.bounds.push_back(INT_MAX);
        c.bucket_index.max_value = 0;
        return results;
    }
    // subsequent runs: partition elements into buckets
    // first search the already indexed buckets
    auto first_bucket_id = GetBucketIDEquiHeight(c.bucket_index.bounds, low);
    auto last_bucket_id = GetBucketIDEquiHeight(c.bucket_index.bounds, high);
    bool check_sortindex = false;
    for(size_t i = first_bucket_id; i <= last_bucket_id; i++) {
        BucketRoot& bucket = c.bucket_index.buckets[i];
        if (bucket.head && !bucket.tail) {
            // we set bucket.tail to NULL when all elements in the bucket have been inserted
            // into the final index
            // hence if bucket.tail is NULL and bucket.head isn't, we know that
            // we can just check the sortindex for this element
            check_sortindex = true;
        } else {
            if (i > first_bucket_id && i < last_bucket_id) {
                bucket.AddAllElements(results);
            } else {
                bucket.RangeQuery(low, high, results);
            }
        }
    }

    if (check_sortindex) {
        range_query_sorted_subsequent_value(c.bucket_index.final_index, c.bucket_index.final_index_entries, low, high, results);
    }

    if (c.bucket_index.index_position >= c.data.size()) {
        // we have finished inserting elements into buckets
        // now we have to sort the data of the buckets into the sorted index
        // we do this using a progressive quicksort

        // loop over the buckets
        ssize_t remaining_swaps = (ssize_t)(c.data.size() * delta);
        while(c.bucket_index.unsorted_bucket < c.bucket_index.buckets.size() && remaining_swaps > 0) {
            auto &bucket = c.bucket_index.buckets[c.bucket_index.unsorted_bucket];
            if (bucket.count == 0) {
                c.bucket_index.unsorted_bucket++;
                continue;
            }
            if (!c.qs_index.index) {
                // index not set
                // first time we are sorting for this bucket
                // initialize the pointers
                c.qs_index.index = &c.sortindex[0] + c.bucket_index.final_index_entries;
                c.qs_index.data = c.bucket_index.final_index + c.bucket_index.final_index_entries;
                c.qs_index.root.start = 0;
                c.qs_index.root.end = bucket.count;
                c.qs_index.root.pivot = (bucket.head->data[0] + bucket.tail->data[bucket.tail->size - 1]) / 2;
                c.qs_index.root.current_start = c.qs_index.root.start;
                c.qs_index.root.current_end = c.qs_index.root.end - 1;
                c.qs_index.root.min = INT_MIN;
                c.qs_index.root.max = INT_MAX;
                c.qs_index.root.sorted = false;
                c.qs_index.root.left = -1;
                c.qs_index.root.right = -1;
                c.qs_index.current_position = 0;

                bucket.sort_entry = bucket.head;
            }

            if (c.qs_index.root.left < 0) {
                int64_t* index = c.qs_index.data;
                size_t* pointers = c.qs_index.index;
                QuicksortNode& node = c.qs_index.root;

                // initialize the elements in the qs index around the pivot
                for(; bucket.sort_entry; bucket.sort_entry = bucket.sort_entry->next) {
                    remaining_swaps -= bucket.sort_entry->size - bucket.sort_entry->sort_index;
                    for(auto& i = bucket.sort_entry->sort_index; i < bucket.sort_entry->size; i++) {
                        int bigger_pivot = bucket.sort_entry->data[i] >= node.pivot;
                        int smaller_pivot = 1 - bigger_pivot;;

                        index[node.current_start] = bucket.sort_entry->data[i];
                        index[node.current_end] = bucket.sort_entry->data[i];
                        pointers[node.current_start] = bucket.sort_entry->indices[i];
                        pointers[node.current_end] = bucket.sort_entry->indices[i];

                        node.current_start += smaller_pivot;
                        node.current_end -= bigger_pivot;
                    }
                    if (remaining_swaps <= 0) {
                        break;
                    }
                }
                if (!bucket.sort_entry) {
                    // finished putting the entire bucket in the initial index
                    // create the children of the root
                    QuicksortNode left(0);
                    left.start = node.start;
                    left.end = index[node.current_start] < node.pivot ?
                               node.current_start + 1 : node.current_start;
                    left.current_start = left.start;
                    left.current_end = left.end - 1;
                    left.pivot = (index[left.current_start] + index[left.current_end]) / 2;
                    left.min = node.min;
                    left.max = node.pivot;
                    QuicksortNode right(1);
                    right.start = left.end;
                    right.end = node.end;
                    right.current_start = right.start;
                    right.current_end = right.end - 1;
                    right.pivot = (index[right.current_start] + index[right.current_end]) / 2;
                    right.min = node.pivot;
                    right.max = node.max;
                    c.qs_index.root.left = c.qs_index.nodes.size();
                    c.qs_index.nodes.push_back(left);
                    c.qs_index.root.right = c.qs_index.nodes.size();
                    c.qs_index.nodes.push_back(right);
                    c.qs_index.current_pivot = 0;
                }
                continue;
            } else {
                // subsequent runs
                // continue the progressive quicksort

                if (c.qs_index.current_pivot < c.qs_index.nodes.size()) {
                    // perform the pivoting operations
                    while(remaining_swaps > 0 && c.qs_index.current_pivot < c.qs_index.nodes.size()) {
                        QuicksortNode& node = c.qs_index.nodes[c.qs_index.current_pivot];
                        if (node.min == node.max) {
                            node.sorted = true;
                            SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent]
                                                            : c.qs_index.root);
                            c.qs_index.current_pivot++;
                        } else if ((node.end - node.start) <= (size_t)(8192 / sizeof(int64_t))) {
                            // node is very small, just sort it normally
                            itqs(c.qs_index.data + node.start, c.qs_index.index + node.start, node.end - node.start);
                            node.sorted = true;
                            SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent]
                                                            : c.qs_index.root);
                            remaining_swaps -= (node.end - node.start) * 13; // log2(8192)
                            c.qs_index.current_pivot++;
                        } else {
                            int64_t *index = c.qs_index.data;
                            size_t* pointers = c.qs_index.index;
                            // now we start swapping stuff
                            while (node.current_start < node.current_end &&
                                   remaining_swaps > 0) {
                                int64_t start = index[node.current_start];
                                int64_t end = index[node.current_end];
                                size_t start_pointer = pointers[node.current_start];
                                size_t end_pointer = pointers[node.current_end];

                                int start_has_to_swap = start >= node.pivot;
                                int end_has_to_swap = end < node.pivot;
                                int has_to_swap = start_has_to_swap * end_has_to_swap;

                                index[node.current_start] =
                                        !has_to_swap * start + has_to_swap * end;
                                index[node.current_end] =
                                        !has_to_swap * end + has_to_swap * start;
                                pointers[node.current_start]
                                        = !has_to_swap * start_pointer +
                                          has_to_swap * end_pointer;
                                pointers[node.current_end]
                                        = !has_to_swap * end_pointer +
                                          has_to_swap * start_pointer;

                                node.current_start += !start_has_to_swap + has_to_swap;
                                node.current_end -= !end_has_to_swap + has_to_swap;
                                remaining_swaps--;
                            }
                            if (node.current_start >= node.current_end) {
                                if (node.current_start == node.start ||
                                    node.current_end == node.end - 1) {
                                    // either the left or right side would have zero entries
                                    // this means either (1) the pivot was chosen poorly
                                    //                or (2) the whole chunk only contains the same value
                                    // otherwise in (1) we pivot again, but move the pivot closer to the min or max
                                    if (node.current_start == node.start) {
                                        assert(node.pivot != node.max);
                                        node.pivot = node.pivot / 2 + node.max / 2;
                                    } else {
                                        assert(node.pivot != node.min);
                                        node.pivot = node.pivot / 2 + node.min / 2;
                                    }
                                    node.current_start = node.start;
                                    node.current_end = node.end - 1;
                                    break;
                                }
                                node.left = c.qs_index.nodes.size();
                                QuicksortNode left(node.left, node.position);
                                left.start = node.start;
                                left.end = index[node.current_start] < node.pivot
                                           ? node.current_start + 1
                                           : node.current_start;
                                left.current_start = left.start;
                                left.current_end = left.end - 1;
                                left.min = node.min;
                                left.max = node.pivot;
                                left.pivot =
                                        (index[left.current_start] + index[left.current_end]) / 2;
                                node.right = c.qs_index.nodes.size() + 1;
                                QuicksortNode right(node.right, node.position);
                                right.start = left.end;
                                right.end = node.end;
                                right.current_start = right.start;
                                right.current_end = right.end - 1;
                                right.pivot =
                                        (index[right.current_start] + index[right.current_end]) / 2;
                                right.min = node.pivot;
                                right.max = node.max;
                                c.qs_index.nodes.push_back(left);
                                c.qs_index.nodes.push_back(right);
                                c.qs_index.current_pivot++;
                            }
                        }
                    }
                }
            }

            if (c.qs_index.root.sorted) {
                // finished sorting! reset all properties and move to the next bucket
                c.qs_index.index = nullptr;
                c.qs_index.data = nullptr;
                c.qs_index.nodes.clear();
                c.qs_index.current_pivot = 0;

                c.bucket_index.final_index_entries += bucket.count;
                c.bucket_index.unsorted_bucket++;
                bucket.tail = nullptr;
            } else {
                break;
            }
        }
    } else {
        size_t next_position = std::min(c.bucket_index.index_position + (size_t)(c.data.size() * delta), c.data.size());
        // now index the elements into the buckets
        for(size_t i = c.bucket_index.index_position; i < next_position; i++) {
            auto bucket_id = GetBucketIDEquiHeight(c.bucket_index.bounds, c.data[i]);
            c.bucket_index.buckets[bucket_id].AddElement(i, c.data[i]);
            int matching = c.data[i] >= low && c.data[i] <= high;
			results.maybe_push_back(c.data[i], matching);
        }
        c.bucket_index.index_position = next_position;
    }

    // now scan the rest of the array to get the rest of the results
    for(size_t i = c.bucket_index.index_position; i < c.data.size(); i++) {
        int matching = c.data[i] >= low && c.data[i] <= high;
			results.maybe_push_back(c.data[i], matching);
    }
    return results;
}

void IncrementalBucketSortIndex::clear() {
    min_value = INT_MAX;
    max_value = INT_MIN;
    buckets.clear();
    index_position = 0;
    unsorted_bucket = 0;
    bounds.clear();
    if (final_index) {
        delete [] final_index;
    }
    final_index = nullptr;
    final_index_entries = 0;
}

void IncrementalBucketSortIndex::Copy(Column& c, IncrementalBucketSortIndex& target) {
    target.min_value = min_value;
    target.max_value = max_value;
    target.buckets.resize(buckets.size());
    for(size_t i = 0; i < buckets.size(); i++) {
        buckets[i].Copy(target.buckets[i]);
    }
    target.bounds = bounds;
    target.index_position = index_position;
    target.unsorted_bucket = unsorted_bucket;
    target.final_index_entries = final_index_entries;
    target.final_index = nullptr;
    if (final_index) {
        target.final_index = new int64_t[c.data.size()];
        memcpy(target.final_index, final_index, c.data.size() * sizeof(int64_t));
    }
}



double get_estimated_time_bucketsort(Column &c, int64_t low, int64_t high, double delta) {
    if (c.converged) {
        auto lower_bound = binary_search_gte(c.bucket_index.final_index, low, 0, c.bucket_index.final_index_entries);
        auto high_bound = binary_search_lte(c.bucket_index.final_index, high, 0, c.bucket_index.final_index_entries);
        double page_count = (high_bound - lower_bound) / ELEMENTS_PER_PAGE;
        double scan_cost = READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS * page_count;
        return (scan_cost + RANDOM_ACCESS_PAGE_MS * log2(c.bucket_index.final_index_entries)) / 1000.0;
    }

    double cost = 0;

    if (c.bucket_index.buckets.size() > 0) {
        bool check_sortindex = false;
        auto first_bucket_id = GetBucketIDEquiHeight(c.bucket_index.bounds, low);
        auto last_bucket_id = GetBucketIDEquiHeight(c.bucket_index.bounds, high);
        for(size_t i = first_bucket_id; i <= last_bucket_id; i++) {
            BucketRoot& bucket = c.bucket_index.buckets[i];
            if (bucket.head && !bucket.tail) {
                // we set bucket.tail to NULL when all elements in the bucket have been inserted
                // into the final index
                // hence if bucket.tail is NULL and bucket.head isn't, we know that
                // we can just check the sortindex for this element
                check_sortindex = true;
            } else {
                if (i > first_bucket_id && i < last_bucket_id) {
                    cost += READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS * bucket.count / ELEMENTS_PER_PAGE;
                } else {
                    cost += READ_ONE_PAGE_SEQ_MS * bucket.count / ELEMENTS_PER_PAGE;
                }
            }
        }

        if (check_sortindex) {
            auto lower_bound = binary_search_gte(c.bucket_index.final_index, low, 0, c.bucket_index.final_index_entries);
            auto high_bound = binary_search_lte(c.bucket_index.final_index, high, 0, c.bucket_index.final_index_entries);
            double page_count = (high_bound - lower_bound) / ELEMENTS_PER_PAGE;
            double scan_cost = READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS * page_count;
            cost += scan_cost += RANDOM_ACCESS_PAGE_MS * log2(c.bucket_index.final_index_entries);
        }

    }

    if (c.bucket_index.index_position < c.data.size()) {
        // initial phase: bucketing
        size_t next_position = std::min(c.bucket_index.index_position + (size_t)(c.data.size() * delta), c.data.size());

        // cost of bucketing
        double bucketing_pages = (next_position - c.bucket_index.index_position) / ELEMENTS_PER_PAGE;
        cost += bucketing_pages * BUCKET_ONE_PAGE_EQUIHEIGHT_MS;

        // base table scan
        double base_table_pages = (c.data.size() - next_position) / ELEMENTS_PER_PAGE;
        cost += base_table_pages * READ_ONE_PAGE_SEQ_MS;
    } else {
        // second phase: quicksort
        // we only do swaps, we don't actually perform any scans/retrieval in the QS
        double page_count = (c.data.size() / ELEMENTS_PER_PAGE) + (c.data.size() % ((int)ELEMENTS_PER_PAGE) != 0 ? 1 : 0);
        double refine_speed = SWAP_COST_PAGE_MS * page_count;
        cost += refine_speed * delta;
    }
    return cost / 1000.0;
}
