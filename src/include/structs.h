#pragma once

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

#define MAXIMUM_BUCKET_ENTRY_SIZE 1024
#define RADIXSORT_LSD_BUCKETS 64

constexpr static int64_t RADIX_MASK = 63;
constexpr static int64_t RADIX_SHIFT = 6;

typedef int bucket_type;
struct Column;

struct ResultStruct {
	int64_t sum = 0;

	void reserve(size_t capacity) {
		(void)capacity;
	}

	size_t size() {
		return 1;
	}

	int64_t *begin() {
		return &sum;
	}

	int64_t *end() {
		return &sum + 1;
	}

	inline void push_back(int64_t value) {
		sum += value;
	}

	inline void maybe_push_back(int64_t value, int maybe) {
		sum += maybe * value;
	}

	inline void merge(ResultStruct other) {
		sum += other.sum;
	}

	int64_t &operator[](const size_t index) {
		return sum;
	}

	const int64_t operator[](const size_t index) const {
		return sum;
	}

	ResultStruct() : sum(0) {
	}
};

struct QuicksortNode {
	int64_t pivot;
	int64_t min = INT_MIN;
	int64_t max = INT_MAX;
	size_t start;
	size_t end;
	size_t current_start, current_end;
	bool sorted;

	int64_t position;
	int64_t parent;
	int64_t left;
	int64_t right;

	QuicksortNode() : position(-1), parent(-1), left(-1), right(-1), sorted(false), min(INT_MIN), max(INT_MAX) {
	}

	QuicksortNode(int64_t position)
	    : position(position), parent(-1), left(-1), right(-1), sorted(false), min(INT_MIN), max(INT_MAX) {
	}

	QuicksortNode(int64_t position, int64_t parent)
	    : position(position), parent(parent), left(-1), right(-1), sorted(false), min(INT_MIN), max(INT_MAX) {
	}
};

struct IncrementalQuicksortIndex {
	std::vector<QuicksortNode> nodes;
	QuicksortNode root;
	size_t *index = nullptr;
	int64_t *data = nullptr;
	size_t current_position = 0;
	size_t current_pivot = 0;

	IncrementalQuicksortIndex() : index(nullptr), data(nullptr), current_pivot(0) {
	}

	void clear();
};

struct BucketEntry {
	size_t size = 0;
	size_t indices[MAXIMUM_BUCKET_ENTRY_SIZE];
	int64_t data[MAXIMUM_BUCKET_ENTRY_SIZE];
	BucketEntry *next = nullptr;
	size_t sort_index = 0;

	BucketEntry *Copy() {
		auto ret = ActualCopy();
		RecursiveCopy(ret, next);
		return ret;
	}

	BucketEntry() : size(0), next(nullptr) {
	}

private:
	void RecursiveCopy(BucketEntry *ret, BucketEntry *entry) {
		if (!entry) {
			return;
		}
		while (entry) {
			auto new_entry = entry->ActualCopy();
			ret->next = new_entry;

			ret = ret->next;
			entry = entry->next;
		}
	}

	BucketEntry *ActualCopy() {
		BucketEntry *ret = new BucketEntry();
		ret->size = size;
		ret->sort_index = sort_index;
		ret->next = nullptr;
		memcpy(ret->indices, indices, MAXIMUM_BUCKET_ENTRY_SIZE * sizeof(size_t));
		memcpy(ret->data, data, MAXIMUM_BUCKET_ENTRY_SIZE * sizeof(int64_t));
		return ret;
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

	void Copy(BucketRoot &other) {
		other.count = count;
		other.head = nullptr;
		other.tail = nullptr;
		other.sort_entry = nullptr;
		if (head) {
			other.head = head->Copy();
			other.tail = other.head;
			while (other.tail->next) {
				other.tail = other.tail->next;
			}

			if (sort_entry) {
				other.sort_entry = other.head;
				auto entry = head;

				while (entry != sort_entry) {
					other.sort_entry = other.sort_entry->next;
					entry = entry->next;
				}
			}
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

	inline void AddAllElements(ResultStruct &results) {
		BucketEntry *head = this->head;
		while (head) {
			for (size_t i = 0; i < head->size; i++) {
				results.push_back(head->data[i]);
			}
			head = head->next;
		}
	}

	inline void RangeQuery(int64_t low, int64_t high, ResultStruct &results) {
		BucketEntry *head = this->head;
		while (head) {
			for (size_t i = 0; i < head->size; i++) {
				int matching = head->data[i] >= low && head->data[i] <= high;
				results.maybe_push_back(head->data[i], matching);
			}
			head = head->next;
		}
	}

	inline void PointQuery(int64_t point, ResultStruct &results) {
		BucketEntry *head = this->head;
		while (head) {
			for (size_t i = 0; i < head->size; i++) {
				int matching = head->data[i] == point;
				results.maybe_push_back(head->data[i], matching);
			}
			head = head->next;
		}
	}
};

struct RadixSortBuckets {
	BucketRoot buckets[RADIXSORT_LSD_BUCKETS];

	RadixSortBuckets *Copy() {
		auto ret = new RadixSortBuckets();
		for (size_t i = 0; i < RADIXSORT_LSD_BUCKETS; i++) {
			buckets[i].Copy(ret->buckets[i]);
		}
		return ret;
	}
};

struct IncrementalRadixIndex {
	RadixSortBuckets *current_buckets = nullptr;
	RadixSortBuckets *prev_buckets = nullptr;

	RadixSortBuckets *final_buckets = nullptr;
	size_t current_bucket_index;
	size_t current_bucket;
	BucketEntry *current_entry = nullptr;
	int iteration;
	int64_t current_power;

	std::vector<int64_t> shifts;
	std::vector<int64_t> masks;

	std::vector<int64_t> final_index;

	IncrementalRadixIndex() : iteration(0), current_power(1), current_bucket_index(0) {
	}

	void Copy(IncrementalRadixIndex &target);
	void clear();
};

struct IncrementalMSDIndex {
	int64_t prev_array_index;
	bool found_boundaries;
	int64_t current_bucket;
	int64_t initial_offset;
	int64_t array_offset;
	int64_t array_offset_bucket;
	int64_t current_bucket_count;
	int64_t prev_bucket_count;
	int64_t shift_index;
	int64_t remaining_buckets;
	BucketEntry *current_entry = nullptr;
	std::unique_ptr<int64_t[]> counts;
	std::unique_ptr<int64_t[]> offsets;
	std::unique_ptr<int64_t[]> data;
	std::unique_ptr<int64_t[]> prev_offsets;
	std::unique_ptr<int64_t[]> prev_array;
	std::unique_ptr<size_t[]> ids;
	std::unique_ptr<size_t[]> prev_ids;

	std::vector<int64_t> shifts;
	std::vector<int64_t> masks;

	IncrementalMSDIndex()
	    : prev_array_index(-1), found_boundaries(false), current_bucket(0), initial_offset(0), array_offset(0),
	      prev_bucket_count(0), current_bucket_count(0) {
	}
	void clear();
	void Copy(Column &c, IncrementalMSDIndex &target);
};

struct IncrementalBucketSortIndex {
	int64_t min_value = INT_MAX;
	int64_t max_value = INT_MIN;
	size_t index_position = 0;
	size_t unsorted_bucket = 0;
	std::vector<int64_t> bounds;

	std::vector<BucketRoot> buckets;

	size_t final_index_entries = 0;
	int64_t *final_index = nullptr;

	IncrementalBucketSortIndex() : final_index(nullptr) {
	}
	void clear();
	void Copy(Column &c, IncrementalBucketSortIndex &target);
};

struct Column {
	int64_t min;
	int64_t max;
	std::vector<int64_t> data;
	std::vector<size_t> sortindex;
	IncrementalBucketSortIndex bucket_index;
	IncrementalMSDIndex msd;
	IncrementalQuicksortIndex qs_index;
	IncrementalRadixIndex radix_index;

	bool converged = false;
	int64_t *final_data = nullptr;

	void Clear() {
		converged = false;
		final_data = nullptr;
		qs_index.clear();
		sortindex.clear();
		radix_index.clear();
		bucket_index.clear();
	};
};

struct RangeQuery {
	std::vector<int64_t> leftpredicate;
	std::vector<int64_t> rightpredicate;
};

class IndexEntry {
public:
	int64_t m_key;
	int64_t m_rowId;

	IndexEntry(int64_t i) : m_key(i), m_rowId(-1) {
	}

	IndexEntry() : m_key(-1), m_rowId(-1) {
	}

	IndexEntry(int64_t key, int64_t rowId) : m_key(key), m_rowId(rowId) {
	}

	//! Query comparisons
	bool operator>(int64_t &other) const {
		return m_key > other;
	}

	bool operator>=(int64_t &other) const {
		return m_key >= other;
	}

	bool operator<(int64_t &other) const {
		return m_key < other;
	}

	bool operator<=(int64_t &other) const {
		return m_key <= other;
	}

	bool operator!=(int64_t &other) const {
		return m_key != other;
	}

	bool operator==(int64_t &other) const {
		return m_key == other;
	}

	bool operator>(const IndexEntry &other) const {
		return m_key > other.m_key;
	}

	bool operator>=(const IndexEntry &other) const {
		return m_key >= other.m_key;
	}

	bool operator<(const IndexEntry &other) const {
		return m_key < other.m_key;
	}

	bool operator<=(const IndexEntry &other) const {
		return m_key <= other.m_key;
	}

	bool operator!=(const IndexEntry &other) const {
		return m_key != other.m_key;
	}
};

struct IndexingTime {
	double index_creation = 0;
};

struct QueryingTime {
	double query_processing = 0;
};

struct TotalTime {

	std::vector<IndexingTime> idx_time;
	std::vector<QueryingTime> q_time;
	std::vector<double> prefix_sum;
	std::vector<double> cost_model;

	void Initialize(size_t query_number) {
		idx_time = std::vector<IndexingTime>(query_number);
		q_time = std::vector<QueryingTime>(query_number);
		prefix_sum = std::vector<double>(query_number);
		cost_model = std::vector<double>(query_number);
	};
};
