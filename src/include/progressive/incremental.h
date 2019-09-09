#pragma once

#include "../structs.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <numeric>
#include <unordered_map>

ResultStruct range_query_incremental_quicksort(Column &c, int64_t low, int64_t high, double delta);

ResultStruct range_query_incremental_bucketsort_equiheight(Column &c, int64_t low, int64_t high, double delta);

ResultStruct range_query_incremental_radixsort_lsd(Column &c, int64_t low, int64_t high, double delta);

ResultStruct range_query_incremental_radixsort_msd(Column &c, int64_t low, int64_t high, double delta);

ResultStruct range_query_incremental_radixsort_msd_noquick(Column &c, int64_t low, int64_t high, double delta);

void SortedCheck(Column &c, QuicksortNode &node);

void VerifyIndex(Column &c, QuicksortNode &node, int min, int max);

void range_query_sorted_subsequent_value(int64_t *index, size_t index_size, int64_t low, int64_t high, int64_t min,
                                         int64_t max, ResultStruct &results);

void range_query_sorted_subsequent_value(int64_t *index, size_t index_size, int64_t low, int64_t high,
                                         ResultStruct &results);

double get_estimated_time_quicksort(Column &c, int64_t low, int64_t high, double delta);

double get_estimated_time_bucketsort(Column &c, int64_t low, int64_t high, double delta);

double get_estimated_time_radixsort_lsd(Column &c, int64_t low, int64_t high, double delta);

double get_estimated_time_radixsort_msd(Column &c, int64_t low, int64_t high, double delta);

double get_estimated_time_radixsort_msd_noquick(Column &c, int64_t low, int64_t high, double delta);

#define PROFILE_BINARY_SEARCH 1
#define PROFILE_BASE_SCAN 2
#define PROFILE_INDEX_SCAN 3
#define PROFILE_INDEX_SWAP 4
#define PROFILE_INDEX_SORT 5

struct Profiler {
	static std::unordered_map<int, double> times;
	static std::unordered_map<int, size_t> tuples;
	static std::unordered_map<int, size_t> scan_counts;
	static std::chrono::time_point<std::chrono::system_clock> start;

	static void Start(int entry) {
		start = std::chrono::system_clock::now();
	}

	static void End(int entry) {
		auto end = std::chrono::system_clock::now();
		times[entry] += std::chrono::duration<double>(end - start).count();
		scan_counts[entry]++;
	}

	static void AddTuples(int entry, size_t count) {
		tuples[entry] += count;
	}

	static void Print() {
		std::cout << "<<Profiler>>\n";
		for (auto &entry : times) {
			const char *name = "Unknown";
			switch (entry.first) {
			case PROFILE_BINARY_SEARCH:
				name = "PROFILE_BINARY_SEARCH";
				break;
			case PROFILE_BASE_SCAN:
				name = "PROFILE_BASE_SCAN";
				break;
			case PROFILE_INDEX_SCAN:
				name = "PROFILE_INDEX_SCAN";
				break;
			case PROFILE_INDEX_SWAP:
				name = "PROFILE_INDEX_SWAP";
				break;
			case PROFILE_INDEX_SORT:
				name = "PROFILE_INDEX_SORT";
				break;
			}
			std::cout << name << ": " << entry.second << "s (Tuples: " << tuples[entry.first]
			          << ", Count: " << scan_counts[entry.first] << ")"
			          << "\n";
		}
	}

	static void Reset() {
		times.clear();
		tuples.clear();
		scan_counts.clear();
	}
};
