
#pragma once

#include "../structs.h"

#include <numeric>
#include <algorithm>
#include <cassert>
#include <cstring>

ResultStruct range_query_incremental_quicksort(Column& c, int64_t low, int64_t high, double delta);


ResultStruct range_query_incremental_cracking(Column& c, int64_t low, int64_t high, double delta);
ResultStruct range_query_incremental_cracking_branched(Column& c, int64_t low, int64_t high, double delta);

double get_max_cracking_delta(Column &c, int64_t low, int64_t high, double delta);

void SortedCheck(Column& c, QuicksortNode& node);
void VerifyIndex(Column& c, QuicksortNode& node, int min, int max);

bool itqs_initialize(int64_t *val, size_t *ind, size_t n, const std::vector<int64_t>& data, size_t offset);

void range_query_sorted_subsequent(const std::vector<int64_t>& array,
								   size_t* index, size_t index_size, int64_t low, int64_t high,
								   int64_t min, int64_t max,
								   ResultStruct& results);

void range_query_sorted_subsequent_value(int64_t* index, size_t index_size, int64_t low, int64_t high,
										 int64_t min, int64_t max,
										 ResultStruct& results);

void range_query_sorted_subsequent_value(int64_t* index, size_t index_size, int64_t low, int64_t high,
										 ResultStruct& results);
