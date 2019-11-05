#include "../include/progressive/constants.h"
#include "../include/progressive/incremental.h"
#include "../include/util/binary_search.h"
#include "../include/util/hybrid_radix_insert_sort.h"

using namespace std;

inline unsigned get_radix_bucket(int64_t value, int iteration) {
	int64_t shift = iteration * RADIX_SHIFT;
	int64_t mask = RADIX_MASK << shift;
	return (value & mask) >> shift;
}

void verify_buckets(Column &c) {
	assert(0);
	std::vector<int64_t> copy;
	copy.resize(c.data.size(), INT_MIN);
	if (c.radix_index.prev_buckets) {
		for (size_t i = 0; i < RADIXSORT_LSD_BUCKETS; i++) {
			BucketEntry *head = c.radix_index.prev_buckets->buckets[i].head;
			while (head) {
				for (size_t j = 0; j < head->size; j++) {
					assert(copy[head->indices[j]] == INT_MIN);
					copy[head->indices[j]] = head->data[j];
				}
				head = head->next;
			}
		}
		assert(std::equal(copy.begin(), copy.end(), c.data.begin()));
	}
}

ResultStruct range_query_incremental_radixsort_lsd(Column &c, int64_t low, int64_t high, double delta) {
	ResultStruct results;
	results.reserve(c.data.size());

	if (!c.radix_index.current_buckets) {
		c.radix_index.current_buckets = new RadixSortBuckets();
	}

	if (c.radix_index.final_index.size() == c.data.size()) {
		c.converged = true;
		c.final_data = &c.radix_index.final_index[0];
		range_query_sorted_subsequent_value(&c.radix_index.final_index[0], c.radix_index.final_index.size(), low, high,
		                                    results);
		return results;
	}

	bool scan_original = false;
	int64_t next_power = c.radix_index.current_power * RADIXSORT_LSD_BUCKETS;
	unsigned l, r;
	if (high - low > next_power) {
		//! no point in searching radix index, we have to scan all the buckets
		scan_original = true;
	} else {
		l = get_radix_bucket(low, c.radix_index.iteration);
		//! assert(l == get_radix_bucket_old(low, c.radix_index.current_power));
		r = get_radix_bucket(high, c.radix_index.iteration);
		//! assert(r == get_radix_bucket_old(high, c.radix_index.current_power));
	}

	if (!c.radix_index.prev_buckets) {
		if (!scan_original) {
			if (l > r) {
				for (unsigned i = 0; i <= r; i++) {
					c.radix_index.current_buckets->buckets[i].RangeQuery(low, high, results);
				}
				for (unsigned i = l; i < RADIXSORT_LSD_BUCKETS; i++) {
					c.radix_index.current_buckets->buckets[i].RangeQuery(low, high, results);
				}
			} else {
				for (unsigned i = l; i <= r; i++) {
					c.radix_index.current_buckets->buckets[i].RangeQuery(low, high, results);
				}
			}
		} else {
			for (size_t i = 0; i < c.radix_index.current_bucket_index; i++) {
				int matching = c.data[i] >= low && c.data[i] <= high;
				results.maybe_push_back(c.data[i], matching);
			}
		}

		//! insert entries from the original array
		size_t next_index =
		    std::min(c.radix_index.current_bucket_index + (size_t)(c.data.size() * delta), c.data.size());
		for (size_t i = c.radix_index.current_bucket_index; i < next_index; i++) {
			unsigned bucket_number = get_radix_bucket(c.data[i], c.radix_index.iteration);
			//! assert(bucket_number == get_radix_bucket_old(c.data[i], c.radix_index.current_power));
			c.radix_index.current_buckets->buckets[bucket_number].AddElement(i, c.data[i]);
			int matching = c.data[i] >= low && c.data[i] <= high;
			results.maybe_push_back(c.data[i], matching);
		}
		c.radix_index.current_bucket_index = next_index;
		for (size_t i = next_index; i < c.data.size(); i++) {
			int matching = c.data[i] >= low && c.data[i] <= high;
			results.maybe_push_back(c.data[i], matching);
		}
		if (next_index == c.data.size()) {
			//! finished with the first iteration
			c.radix_index.prev_buckets = c.radix_index.current_buckets;
			c.radix_index.current_buckets = nullptr;
			c.radix_index.current_bucket = 0;
			c.radix_index.current_entry = c.radix_index.prev_buckets->buckets[0].head;
			//! verify_buckets(c);
		}
	} else {
		//! next iterations
		//! move from one bucket to the next
		//! first search the current set of buckets
		if (!scan_original) {
			if (l > r) {
				for (unsigned i = 0; i <= r; i++) {
					c.radix_index.prev_buckets->buckets[i].RangeQuery(low, high, results);
				}
				for (unsigned i = l; i < RADIXSORT_LSD_BUCKETS; i++) {
					c.radix_index.prev_buckets->buckets[i].RangeQuery(low, high, results);
				}
			} else {
				for (unsigned i = l; i <= r; i++) {
					c.radix_index.prev_buckets->buckets[i].RangeQuery(low, high, results);
				}
			}
		}
		size_t remaining_elements = (size_t)(c.data.size() * delta);
		if (c.radix_index.final_buckets) {
			//! final sorted buckets, copy into ordered index
			while (c.radix_index.current_bucket < RADIXSORT_LSD_BUCKETS) {
				while (c.radix_index.current_entry) {
					for (size_t i = 0; i < c.radix_index.current_entry->size; i++) {
						c.sortindex.push_back(c.radix_index.current_entry->indices[i]);
						c.radix_index.final_index.push_back(c.radix_index.current_entry->data[i]);
					}

					if (c.radix_index.current_entry->size >= remaining_elements) {
						remaining_elements = 0;
					} else {
						remaining_elements -= c.radix_index.current_entry->size;
					}
					c.radix_index.current_entry = c.radix_index.current_entry->next;
					if (remaining_elements == 0)
						break;
				}
				if (remaining_elements == 0) {
					break;
				}
				c.radix_index.current_bucket++;
				if (c.radix_index.current_bucket < RADIXSORT_LSD_BUCKETS) {
					c.radix_index.current_entry =
					    c.radix_index.prev_buckets->buckets[c.radix_index.current_bucket].head;
				}
			}
			if (c.radix_index.final_index.size() == c.data.size()) {
				//! delete c.radix_index.prev_buckets;
				c.radix_index.prev_buckets = nullptr;
				c.radix_index.final_buckets = nullptr;
			}
		} else {
			//! move elements from the current bucket to the next
			while (c.radix_index.current_bucket < RADIXSORT_LSD_BUCKETS) {
				while (c.radix_index.current_entry) {
					for (size_t i = 0; i < c.radix_index.current_entry->size; i++) {
						unsigned bucket_number =
						    get_radix_bucket(c.radix_index.current_entry->data[i], c.radix_index.iteration + 1);
						//! assert(bucket_number == get_radix_bucket_old(c.radix_index.current_entry->data[i],
						//! next_power));
						c.radix_index.current_buckets->buckets[bucket_number].AddElement(
						    c.radix_index.current_entry->indices[i], c.radix_index.current_entry->data[i]);
					}

					if (c.radix_index.current_entry->size >= remaining_elements) {
						remaining_elements = 0;
					} else {
						remaining_elements -= c.radix_index.current_entry->size;
					}
					c.radix_index.current_entry = c.radix_index.current_entry->next;
					if (remaining_elements == 0)
						break;
				}
				if (remaining_elements == 0) {
					break;
				}
				c.radix_index.current_bucket++;
				if (c.radix_index.current_bucket < RADIXSORT_LSD_BUCKETS) {
					c.radix_index.current_entry =
					    c.radix_index.prev_buckets->buckets[c.radix_index.current_bucket].head;
				}
			}
			if (c.radix_index.current_bucket == RADIXSORT_LSD_BUCKETS) {
				c.radix_index.iteration++;
				c.radix_index.current_power = next_power;
				c.radix_index.current_bucket = 0;
				//! delete c.radix_index.prev_buckets;
				c.radix_index.prev_buckets = c.radix_index.current_buckets;
				c.radix_index.current_entry = c.radix_index.prev_buckets->buckets[0].head;
				//! required iteration is log2(max - min) / log2(RADIXSORT_LSD_BUCKETS)
				int64_t required_iterations = ceil(log2(c.max - c.min) / log2(RADIXSORT_LSD_BUCKETS));
				if (c.radix_index.iteration == required_iterations) { //! 256^3, we assume data is <2^32
					//! finished with initial indexing
					c.radix_index.final_buckets = c.radix_index.current_buckets;
					c.radix_index.final_index.reserve(c.data.size());
					c.sortindex.reserve(c.data.size());
				} else {
					c.radix_index.prev_buckets = c.radix_index.current_buckets;
					c.radix_index.current_buckets = nullptr;
				}
				//! verify_buckets(c);
			}
		}

		if (scan_original) {
			for (size_t i = 0; i < c.data.size(); i++) {
				int matching = c.data[i] >= low && c.data[i] <= high;
				results.maybe_push_back(c.data[i], matching);
			}
		}
	}
	return results;
}

double get_estimated_time_radixsort_lsd(Column &c, int64_t low, int64_t high, double delta) {
	if (c.radix_index.final_index.size() == c.data.size()) {
		auto lower_bound = binary_search_gte(&c.radix_index.final_index[0], low, 0, c.radix_index.final_index.size());
		auto high_bound = binary_search_lte(&c.radix_index.final_index[0], high, 0, c.radix_index.final_index.size());
		double page_count = (high_bound - lower_bound) / ELEMENTS_PER_PAGE;
		double scan_cost = READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS * page_count;
		return (scan_cost + 2 * RANDOM_ACCESS_PAGE_MS * log2(c.radix_index.final_index.size())) / 1000.0;
	}

	bool scan_original = false;
	int64_t next_power = c.radix_index.current_power * RADIXSORT_LSD_BUCKETS;
	unsigned l, r;
	if (!c.radix_index.current_buckets || high - low > next_power) {
		//! no point in searching radix index, we have to scan all the buckets
		scan_original = true;
	} else {
		l = get_radix_bucket(low, c.radix_index.current_power);
		r = get_radix_bucket(high, c.radix_index.current_power);
	}
	double page_count = (c.data.size() / ELEMENTS_PER_PAGE) + (c.data.size() % ((int)ELEMENTS_PER_PAGE) != 0 ? 1 : 0);
	double cost = 0;
	if (scan_original) {
		//! scan original cost
		cost += READ_ONE_PAGE_SEQ_MS * page_count;
	} else {
		//! cost to search through buckets
		size_t elements = 0;
		if (!c.radix_index.prev_buckets) {
			if (l > r) {
				for (unsigned i = 0; i <= r; i++) {
					elements += c.radix_index.current_buckets->buckets[i].count;
				}
				for (unsigned i = l; i < RADIXSORT_LSD_BUCKETS; i++) {
					elements += c.radix_index.current_buckets->buckets[i].count;
				}
			} else {
				for (unsigned i = l; i <= r; i++) {
					elements += c.radix_index.current_buckets->buckets[i].count;
				}
			}
		} else {
			if (l > r) {
				for (unsigned i = 0; i <= r; i++) {
					elements += c.radix_index.prev_buckets->buckets[i].count;
				}
				for (unsigned i = l; i < RADIXSORT_LSD_BUCKETS; i++) {
					elements += c.radix_index.prev_buckets->buckets[i].count;
				}
			} else {
				for (unsigned i = l; i <= r; i++) {
					elements += c.radix_index.prev_buckets->buckets[i].count;
				}
			}
		}
		cost += (elements / ELEMENTS_PER_PAGE) * READ_ONE_PAGE_SEQ_MS;
	}

	//! bucketing cost
	if (c.radix_index.final_buckets) {
		cost += delta * LSD_RADIXSORT_COPY_ONE_PAGE_MS * page_count;
	} else {
		cost += delta * LSD_RADIXSORT_BUCKETING_PAGE_MS * page_count;
	}
	return cost / 1000.0;
}

void IncrementalRadixIndex::clear() {
	current_buckets = nullptr;
	prev_buckets = nullptr;
	final_buckets = nullptr;
	iteration = 0;
	current_power = 1;
	current_bucket_index = 0;
	final_index.clear();
}

void IncrementalRadixIndex::Copy(IncrementalRadixIndex &target) {
	target.current_buckets = current_buckets ? current_buckets->Copy() : nullptr;
	target.prev_buckets = prev_buckets ? prev_buckets->Copy() : nullptr;
	target.final_buckets = final_buckets ? final_buckets->Copy() : nullptr;
	if (current_entry) {
		//! find the current_entry
		if (prev_buckets) {
			auto start = prev_buckets->buckets[current_bucket].head;
			target.current_entry = target.prev_buckets->buckets[current_bucket].head;
			while (start != current_entry) {
				start = start->next;
				target.current_entry = target.current_entry->next;
			}
		}
	}

	target.current_power = current_power;
	target.current_bucket = current_bucket;
	target.current_bucket_index = current_bucket_index;
	target.final_index = final_index;
}
