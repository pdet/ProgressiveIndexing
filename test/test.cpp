#define CATCH_CONFIG_MAIN //! This tells Catch to provide a main() - only do this in one cpp file

#include "../src/include/progressive/incremental.h"
#include "../src/include/progressive/progressive_indexing.h"
#include "../src/include/structs.h"

#include <algorithm>
#include <assert.h>
#include <catch.hpp>
#include <cstdlib>
#include <iostream>
#include <stdio.h>

#pragma clang diagnostic ignored "-Wformat"

using namespace std;

int64_t COLUMN_SIZE = 10000000;
int64_t NUM_QUERIES = 100;
int INTERACTIVITY_IS_PERCENTAGE = 1;
int RADIXSORT_TOTAL_BYTES;
double DELTA = 0.2;
double INTERACTIVITY_THRESHOLD = 1.2;
TotalTime query_times;
size_t current_query;
int FIXED_BUDGET = 1;

void generate_column(vector<int64_t> &data) {
	for (int i = 0; i < COLUMN_SIZE; i++) {
		data.push_back(i);
	}
	random_shuffle(data.begin(), data.end());
}

bool generate_workload(int S, vector<int64_t> &leftQuery, vector<int64_t> &rightQuery) {
	for (size_t i = 0; i < NUM_QUERIES; i++) {
		leftQuery.push_back(rand() % (COLUMN_SIZE - S));
		rightQuery.push_back(leftQuery[i] + S);
	}
}

TEST_CASE("Check all algorithms under Uniform Random", "[Random]") {
	Column c;
	RangeQuery rangequeries;
	vector<int64_t> answers;
	vector<double> deltas(NUM_QUERIES);
	query_times.Initialize(NUM_QUERIES);

	//! Generate Column 10^7
	generate_column(c.data);

	//! Generate Workload 1000 Queries 0.01 Selectivity
	generate_workload(1000, rangequeries.leftpredicate, rangequeries.rightpredicate);

	//! Generate Query Answers
	for (size_t q_it = 0; q_it < NUM_QUERIES; q_it++) {
		int64_t sum = 0;
		for (size_t col_it = 0; col_it < COLUMN_SIZE; col_it++) {
			if (c.data[col_it] >= rangequeries.leftpredicate[q_it] &&
			    c.data[col_it] <= rangequeries.rightpredicate[q_it]) {
				sum += c.data[col_it];
			}
		}
		answers.push_back(sum);
	}

	//! Run Workload on all algorithms FIXED_DELTA = 0.2
	fprintf(stderr, "Testing P. QuickSort Fixed Delta\n", current_query);
	progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_quicksort);
	c.Clear();
	fprintf(stderr, "Testing P. BucketSort Fixed Delta\n", current_query);
	progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_bucketsort_equiheight);
	c.Clear();
	fprintf(stderr, "Testing P. Radixsort LSD Fixed Delta\n", current_query);
	progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_lsd);
	c.Clear();
    fprintf(stderr, "Testing P. Radixsort MSD Fixed Delta\n", current_query);
    progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_msd_noquick);
    c.Clear();

	//! Run Workload on all algorithms INTERACTIVITY_THRESHOLD = 1.2*FS
	fprintf(stderr, "Testing P. QuickSort Variable Delta\n", current_query);

	progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_quicksort,
	                                get_estimated_time_quicksort);
	c.Clear();

	fprintf(stderr, "Testing P. BucketSort Variable Delta\n", current_query);
	progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_bucketsort_equiheight,
	                                get_estimated_time_bucketsort);
	c.Clear();

	fprintf(stderr, "Testing P. Radixsort LSD Variable Delta\n", current_query);
	progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_radixsort_lsd,
	                                get_estimated_time_radixsort_lsd);
	c.Clear();
	fprintf(stderr, "Testing P. Radixsort MSD Variable Delta\n", current_query);

	progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_radixsort_msd_noquick,
		                                get_estimated_time_radixsort_msd_noquick);
}
