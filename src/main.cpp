#include "include/progressive/incremental.h"
#include "include/progressive/progressive_indexing.h"
#include "include/util/file_manager.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <vector>

#pragma clang diagnostic ignored "-Wformat"

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
int64_t COLUMN_SIZE;
int64_t NUM_QUERIES;
int ALGORITHM, INTERACTIVITY_IS_PERCENTAGE, RADIXSORT_TOTAL_BYTES;
double DELTA, INTERACTIVITY_THRESHOLD;
TotalTime query_times;
size_t current_query;
int FIXED_BUDGET;

void print_help(int argc, char **argv) {
	fprintf(stderr, "Unrecognized command line option.\n");
	fprintf(stderr, "Usage: %s [args]\n", argv[0]);
	fprintf(stderr, "   --column-path\n");
	fprintf(stderr, "   --query-path\n");
	fprintf(stderr, "   --answers-path\n");
	fprintf(stderr, "   --num-queries\n");
	fprintf(stderr, "   --column-size\n");
	fprintf(stderr, "   --algorithm\n");
	fprintf(stderr, "   --delta\n");
	fprintf(stderr, "   --interactivity-threshold\n");
	fprintf(stderr, "   --interactivity-is-percentage\n");
	fprintf(stderr, "   --fxd-budget\n");
}

pair<string, string> split_once(string delimited, char delimiter) {
	auto pos = delimited.find_first_of(delimiter);
	return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {
	COLUMN_FILE_PATH = "generated_data/10000000/column";
	QUERIES_FILE_PATH = "generated_data/10000000/query_1_2";
	ANSWER_FILE_PATH = "generated_data/10000000/answer_1_2";
	NUM_QUERIES = 10000;
	COLUMN_SIZE = 10000000;
	ALGORITHM = 1;
	DELTA = 0.1;
	INTERACTIVITY_IS_PERCENTAGE = 1; //! By default interactivity is a percentage of FS time
	INTERACTIVITY_THRESHOLD = 1.2;
	FIXED_BUDGET = 0; //! By Default we run adaptive indexing budgets

	int repetition = 10;

	for (int i = 1; i < argc; i++) {
		auto arg = string(argv[i]);
		if (arg.substr(0, 2) != "--") {
			print_help(argc, argv);
			exit(EXIT_FAILURE);
		}
		arg = arg.substr(2);
		auto p = split_once(arg, '=');
		auto &arg_name = p.first;
		auto &arg_value = p.second;
		if (arg_name == "column-path") {
			COLUMN_FILE_PATH = arg_value;
		} else if (arg_name == "query-path") {
			QUERIES_FILE_PATH = arg_value;
		} else if (arg_name == "answer-path") {
			ANSWER_FILE_PATH = arg_value;
		} else if (arg_name == "num-queries") {
			NUM_QUERIES = atoi(arg_value.c_str());
		} else if (arg_name == "column-size") {
			COLUMN_SIZE = atoi(arg_value.c_str());
		} else if (arg_name == "algorithm") {
			ALGORITHM = atoi(arg_value.c_str());
		} else if (arg_name == "delta") {
			DELTA = atof(arg_value.c_str());
		} else if (arg_name == "interactivity-is-percentage") {
			INTERACTIVITY_IS_PERCENTAGE = atoi(arg_value.c_str());
		} else if (arg_name == "interactivity-threshold") {
			INTERACTIVITY_THRESHOLD = atof(arg_value.c_str());
		} else if (arg_name == "fxd-budget") {
			FIXED_BUDGET = atoi(arg_value.c_str());
		} else {
			print_help(argc, argv);
			exit(EXIT_FAILURE);
		}
	}

	RADIXSORT_TOTAL_BYTES = ceil(log2(COLUMN_SIZE));
	RangeQuery rangequeries;
	load_queries(&rangequeries, QUERIES_FILE_PATH, NUM_QUERIES);
	Column c;
	load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);

	vector<int64_t> answers;
	load_answers(&answers, ANSWER_FILE_PATH, NUM_QUERIES);
	query_times.Initialize(NUM_QUERIES);
	vector<double> times(NUM_QUERIES);
	vector<double> deltas(NUM_QUERIES);
	current_query = 0;
	for (size_t i = 0; i < repetition; i++) {
		switch (ALGORITHM) {
		case 1:
			progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_quicksort);
			break;
		case 2:
			progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_quicksort,
			                                get_estimated_time_quicksort);
			break;
		case 3:
			progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_bucketsort_equiheight);
			break;
		case 4:
			progressive_indexing_cost_model(c, rangequeries, answers, deltas,
			                                range_query_incremental_bucketsort_equiheight,
			                                get_estimated_time_bucketsort);
			break;
		case 5:
			progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_lsd);
			break;
		case 6:
			progressive_indexing_cost_model(c, rangequeries, answers, deltas, range_query_incremental_radixsort_lsd,
			                                get_estimated_time_radixsort_lsd);
			break;
		case 7:
			progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_msd_noquick);
			break;

		case 8:
			progressive_indexing_cost_model(c, rangequeries, answers, deltas,
			                                range_query_incremental_radixsort_msd_noquick,
			                                get_estimated_time_radixsort_msd_noquick);
			break;
		}
	}
	for (size_t i = 0; i < NUM_QUERIES; i++) {
		double total_time, total_indexing, total_querying;
		total_time = query_times.idx_time[i].index_creation + query_times.q_time[i].query_processing;
		cout << deltas[i] / repetition << ";" << query_times.q_time[i].query_processing / repetition << ";"
		     << query_times.idx_time[i].index_creation / repetition << ";" << total_time / repetition << ";"
		     << query_times.prefix_sum[i] / repetition << ";" << query_times.cost_model[i] / repetition << "\n";
	}
}
