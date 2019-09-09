#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define RADIXSORT_MSD_BYTES 8
//! total amount of relevant bytes in the data set
//! this is ceil(log2(MAX_VALUE)) of the data set
//! since our max value is 10^8, ceil(log2(10^8)) = 27
//#define RADIXSORT_TOTAL_BYTES 27
int RADIXSORT_TOTAL_BYTES;
#define MAXIMUM_BUCKET_ENTRY_SIZE 1024

#define PAGESIZE 4096
#define ELEMENTS_PER_PAGE (PAGESIZE / sizeof(int64_t))

#define PAGES_TO_WRITE 100000

#define ELEMENT_COUNT (PAGES_TO_WRITE * ELEMENTS_PER_PAGE)
#define EQUIHEIGHT_BUCKET_COUNT 128
#define EQUIHEIGHT_MID_POINT 64
#define EQUIHEIGHT_75 96
#define EQUIHEIGHT_25 32
#define INCREMENTAL_RADIX_BASE 256
static inline int GetBucketIDRadixSort(int64_t point) {
	return point >> (RADIXSORT_TOTAL_BYTES - RADIXSORT_MSD_BYTES);
}

struct BucketEntry {
	size_t size = 0;
	size_t indices[MAXIMUM_BUCKET_ENTRY_SIZE];
	int64_t data[MAXIMUM_BUCKET_ENTRY_SIZE];
	BucketEntry *next = nullptr;
	size_t sort_index = 0;

	BucketEntry() : size(0), next(nullptr) {
	}
};

struct BucketRoot {
	BucketEntry *head = nullptr;
	BucketEntry *tail = nullptr;

	BucketEntry *sort_entry = nullptr;
	size_t count = 0;

	BucketRoot() : head(nullptr), tail(nullptr), sort_entry(nullptr), count(0) {
	}
	~BucketRoot() {
		while (head) {
			BucketEntry *next = head->next;
			delete head;
			head = next;
		}
	}

	inline void AddElement(size_t index, int64_t value) {
		if (!tail) {
			tail = new BucketEntry();
			head = tail;
		}
		if (tail->size >= MAXIMUM_BUCKET_ENTRY_SIZE) {
			tail->next = new BucketEntry();
			tail = tail->next;
		}
		count++;
		tail->data[tail->size] = value;
		tail->indices[tail->size++] = index;
	}
};

static inline int OldGetBucketIDEquiHeight(std::vector<int64_t> &bounds, int64_t point) {
	size_t bucket = 0;
	for (size_t i = 0; i < bounds.size(); i++) {
		bucket += point > bounds[i];
	}
	return bucket;
}

static inline int GetBucketIDEquiHeight(std::vector<int64_t> &bounds, int64_t point) {
	int bucket = 0;
	if (point > bounds[EQUIHEIGHT_MID_POINT]) {
		if (point > bounds[EQUIHEIGHT_75]) {
			for (size_t i = EQUIHEIGHT_75; i < bounds.size(); i++) {
				bucket += point > bounds[i];
			}
			return bucket + EQUIHEIGHT_75;
		} else {
			for (size_t i = EQUIHEIGHT_MID_POINT; i < EQUIHEIGHT_75; i++) {
				bucket += point > bounds[i];
			}
			return bucket + EQUIHEIGHT_MID_POINT;
		}
	} else {
		if (point > bounds[EQUIHEIGHT_25]) {
			for (size_t i = EQUIHEIGHT_25; i < EQUIHEIGHT_MID_POINT; i++) {
				bucket += point > bounds[i];
			}
			return bucket + EQUIHEIGHT_25;
		} else {
			for (size_t i = 0; i < EQUIHEIGHT_25; i++) {
				bucket += point > bounds[i];
			}
			return bucket;
		}
	}
}

inline unsigned get_radix_bucket(int64_t value, int64_t power) {
	return (unsigned)((value / power) % INCREMENTAL_RADIX_BASE);
}

int main(int argc, char **argv) {
	int64_t *base_column = new int64_t[PAGES_TO_WRITE * ELEMENTS_PER_PAGE];
	int64_t *values = new int64_t[PAGES_TO_WRITE * ELEMENTS_PER_PAGE];
	size_t *index = new size_t[ELEMENT_COUNT];
	ssize_t remaining_swaps;
	int COLUMN_SIZE;
	if (argc > 1) {
		COLUMN_SIZE = atoi(argv[1]);
	}
	RADIXSORT_TOTAL_BYTES = ceil(log2(COLUMN_SIZE));
	struct {
		size_t current_start;
		size_t current_end;
		size_t pivot = 5000;
	} node;
	node.current_start = 0;
	node.current_end = ELEMENT_COUNT - 1;
	int64_t low = 1000;
	int64_t high = 20000;
	int64_t sum = 0;

	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		base_column[i] = rand() % 50000;
	}

	//! initial write
	auto start = std::chrono::system_clock::now();
	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		int matching = base_column[i] >= low && base_column[i] <= high;
		sum += matching * base_column[i];

		int bigger_pivot = base_column[i] >= node.pivot;
		int smaller_pivot = 1 - bigger_pivot;

		values[node.current_start] = base_column[i];
		values[node.current_end] = base_column[i];
		index[node.current_start] = base_column[i];
		index[node.current_end] = base_column[i];

		node.current_start += smaller_pivot;
		node.current_end -= bigger_pivot;
	}
	auto end = std::chrono::system_clock::now();

	double s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define WRITE_ONE_PAGE_SEQ_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";
	//! reading
	start = std::chrono::system_clock::now();
	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		int matching = values[i] >= low && values[i] <= high;
		sum += values[i] * matching;
	}
	end = std::chrono::system_clock::now();
	//! Avoiding -O3 Optimization
	if (sum != 0)
		fprintf(stderr, " ");
	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define READ_ONE_PAGE_SEQ_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	//! reading
	start = std::chrono::system_clock::now();
	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		sum += values[i];
	}
	end = std::chrono::system_clock::now();
	//! Avoiding -O3 Optimization
	if (sum != 0)
		fprintf(stderr, " ");
	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	std::vector<int> random_lookups;
	for (size_t i = 0; i < PAGES_TO_WRITE; i++) {
		random_lookups.push_back(rand() % PAGES_TO_WRITE);
	}

	//! random page access
	start = std::chrono::system_clock::now();
	for (size_t i = 0; i < PAGES_TO_WRITE; i++) {
		sum += values[random_lookups[i]];
	}
	end = std::chrono::system_clock::now();
	//! Avoiding -O3 Optimization
	if (sum != 0)
		fprintf(stderr, " ");
	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define RANDOM_ACCESS_PAGE_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		values[i] = rand() % 10000;
		index[i] = i;
	}

	node.current_start = 0;
	node.current_end = ELEMENT_COUNT - 1;

	//! swapping
	remaining_swaps = ELEMENT_COUNT;
	start = std::chrono::system_clock::now();
	while (node.current_start < node.current_end && remaining_swaps > 0) {
		int64_t start = values[node.current_start];
		int64_t end = values[node.current_end];
		size_t start_pointer = index[node.current_start];
		size_t end_pointer = index[node.current_end];

		int start_has_to_swap = start >= node.pivot;
		int end_has_to_swap = end < node.pivot;
		int has_to_swap = start_has_to_swap * end_has_to_swap;

		values[node.current_start] = !has_to_swap * start + has_to_swap * end;
		values[node.current_end] = !has_to_swap * end + has_to_swap * start;
		index[node.current_start] = !has_to_swap * start_pointer + has_to_swap * end_pointer;
		index[node.current_end] = !has_to_swap * end_pointer + has_to_swap * start_pointer;

		node.current_start += !start_has_to_swap + has_to_swap;
		node.current_end -= !end_has_to_swap + has_to_swap;
		remaining_swaps--;
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define SWAP_COST_PAGE_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	std::vector<BucketRoot> buckets;
	buckets.resize(pow(2, RADIXSORT_MSD_BYTES));

	size_t bound = pow(2, RADIXSORT_TOTAL_BYTES);
	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		base_column[i] = rand() % bound;
	}

	start = std::chrono::system_clock::now();
	for (auto i = 0; i < ELEMENT_COUNT; i++) {
		auto bucket_id = GetBucketIDRadixSort(base_column[i]);
		buckets[bucket_id].AddElement(i, base_column[i]);
		int matching = base_column[i] >= low && base_column[i] <= high;
		sum += values[i] * matching;
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define BUCKET_ONE_PAGE_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	std::vector<int64_t> bounds;
	for (size_t i = 0; i < EQUIHEIGHT_BUCKET_COUNT; i++) {
		bounds.push_back(base_column[i]);
	}
	std::sort(bounds.begin(), bounds.end());
	buckets.clear();
	buckets.resize(EQUIHEIGHT_BUCKET_COUNT + 1);

	start = std::chrono::system_clock::now();
	for (auto i = 0; i < ELEMENT_COUNT; i++) {
		auto bucket_id = GetBucketIDEquiHeight(bounds, base_column[i]);
		buckets[bucket_id].AddElement(i, base_column[i]);
		int matching = base_column[i] >= low && base_column[i] <= high;
		sum += values[i] * matching;
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define BUCKET_ONE_PAGE_EQUIHEIGHT_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	int power = 1;

	buckets.clear();
	buckets.resize(INCREMENTAL_RADIX_BASE);
	start = std::chrono::system_clock::now();
	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		unsigned bucket_number = get_radix_bucket(base_column[i], power);
		buckets[bucket_number].AddElement(i, base_column[i]);
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define LSD_RADIXSORT_BUCKETING_PAGE_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	size_t index_copy[ELEMENT_COUNT];
	int64_t value_copy[ELEMENT_COUNT];
	//! copy from buckets
	int entry = 0;
	int current_bucket = 0;
	size_t remaining_elements = ELEMENT_COUNT;
	BucketEntry *current_entry = buckets[0].head;

	start = std::chrono::system_clock::now();
	while (current_bucket < INCREMENTAL_RADIX_BASE) {
		while (current_entry) {
			for (size_t i = 0; i < current_entry->size; i++) {
				index_copy[entry] = current_entry->indices[i];
				index_copy[entry++] = current_entry->data[i];
			}

			if (current_entry->size >= remaining_elements) {
				remaining_elements = 0;
			} else {
				remaining_elements -= current_entry->size;
			}
			current_entry = current_entry->next;
			if (remaining_elements == 0)
				break;
		}

		if (remaining_elements == 0) {
			break;
		}
		current_bucket++;
		if (current_bucket < INCREMENTAL_RADIX_BASE) {
			current_entry = buckets[current_bucket].head;
		}
	}

	for (size_t i = 0; i < ELEMENT_COUNT; i++) {
		unsigned bucket_number = get_radix_bucket(base_column[i], power);
		buckets[bucket_number].AddElement(i, base_column[i]);
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
	std::cout << "#define LSD_RADIXSORT_COPY_ONE_PAGE_MS " << (s * 1000) / PAGES_TO_WRITE << "\n";

	power *= INCREMENTAL_RADIX_BASE;

	remaining_elements = ELEMENT_COUNT;
	current_entry = buckets[0].head;
	current_bucket = 0;
	std::vector<BucketRoot> next_buckets;
	next_buckets.resize(buckets.size());

	start = std::chrono::system_clock::now();
	while (current_bucket < INCREMENTAL_RADIX_BASE) {
		while (current_entry) {
			for (size_t i = 0; i < current_entry->size; i++) {
				unsigned bucket_number = get_radix_bucket(current_entry->data[i], power);
				next_buckets[bucket_number].AddElement(current_entry->indices[i], current_entry->data[i]);
			}

			if (current_entry->size >= remaining_elements) {
				remaining_elements = 0;
			} else {
				remaining_elements -= current_entry->size;
			}
			current_entry = current_entry->next;
			if (remaining_elements == 0)
				break;
		}
		if (remaining_elements == 0) {
			break;
		}
		current_bucket++;
		if (current_bucket < INCREMENTAL_RADIX_BASE) {
			current_entry = buckets[current_bucket].head;
		}
	}
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
}
