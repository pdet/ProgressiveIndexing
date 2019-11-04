#include "include/progressive/incremental.h"
#include "include/setup.h"
#include "include/structs.h"
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

typedef ResultStruct (*progressive_function)(Column &c, int64_t low, int64_t high, double delta);

typedef double (*estimate_function)(Column &c, int64_t low, int64_t high, double delta);

void progressive_indexing(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &deltas,
                          progressive_function function) {
	chrono::time_point<chrono::system_clock> start, end;
	int64_t sum = 0;
	bool converged = false;
	double prefix_sum = 0;

	if (function == range_query_incremental_bucketsort_equiheight) {
		//! create the initial buckets for bucketsort without counting the time
		//! we do this because it throws off our benchmarking otherwise
		//! otherwise we spent one iteration on determining min,max of the column
		//! which is unrelated to the delta of the algorithm or anything else
		//! we assume the database knows these from statistics in the paper
		//! so we just run a "warm up run" to determine these here prior to benchmarking
		function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
	}

	if (function == range_query_incremental_radixsort_msd ||
	    function == range_query_incremental_bucketsort_equiheight) {
		//! initialize the sort index
		column.sortindex.resize(column.data.size());
		column.bucket_index.final_index = new int64_t[column.data.size()];
		column.bucket_index.final_index_entries = 0;
	}

	for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
		ResultStruct results;
		start = chrono::system_clock::now();
		results =
		    function(column, rangeQueries.leftpredicate[current_query], rangeQueries.rightpredicate[current_query], 0);
		end = chrono::system_clock::now();
		double baseline = chrono::duration<double>(end - start).count();
		query_times.q_time[current_query].query_processing += baseline;
		if (results.sum != answers[current_query])
			fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
			        answers[current_query], sum);
		start = chrono::system_clock::now();

		results = function(column, rangeQueries.leftpredicate[current_query],
		                   rangeQueries.rightpredicate[current_query], DELTA);
		end = chrono::system_clock::now();

		sum = results.sum;
		prefix_sum += chrono::duration<double>(end - start).count();
		query_times.prefix_sum[current_query] += prefix_sum;
		query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count() - baseline;

		if (!converged && column.converged) {
			fprintf(stderr, "Converged on query %lld\n", current_query);
			converged = true;
		}
		deltas[current_query] += DELTA;
	}
}

void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                     vector<double> &deltas, progressive_function function,
                                     estimate_function estimate) {
	chrono::time_point<chrono::system_clock> start, end;
	int64_t sum = 0;
	bool converged = false;
	if (function == range_query_incremental_bucketsort_equiheight) {
		//! create the initial buckets for bucketsort without counting the time
		//! we do this because it throws off our benchmarking otherwise
		//! otherwise we spent one iteration on determining min,max of the column
		//! which is unrelated to the delta of the algorithm or anything else
		//! we assume the database knows these from statistics in the paper
		//! so we just run a "warm up run" to determine these here prior to benchmarking
		function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
	}

	if (function == range_query_incremental_radixsort_msd ||
	    function == range_query_incremental_bucketsort_equiheight) {
		//! initialize the sort index
		column.sortindex.resize(column.data.size());
		column.bucket_index.final_index = new int64_t[column.data.size()];
		column.bucket_index.final_index_entries = 0;
	}
	ResultStruct results;

	//! 0.5s
	//! Run Dummy Full Scan to check if its higher or lower than the interactivity threshold
	//! first get the base time with delta = 0
	start = chrono::system_clock::now();
	results = function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], 0);
	end = chrono::system_clock::now();
	if (results.sum != answers[0]) {
		fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", 0, answers[0], sum);
	}
	double full_scan_time = chrono::duration<double>(end - start).count();
	double best_convergence_delta_time = 1.5 * full_scan_time;
	double ratio, initial_query_time, final_query_time;
	double prefix_sum = 0;

	if (INTERACTIVITY_IS_PERCENTAGE)
		INTERACTIVITY_THRESHOLD = INTERACTIVITY_THRESHOLD * full_scan_time;
	for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
		if (full_scan_time > INTERACTIVITY_THRESHOLD) {
			start = chrono::system_clock::now();
			results = function(column, rangeQueries.leftpredicate[current_query],
			                   rangeQueries.rightpredicate[current_query], 0);
			end = chrono::system_clock::now();
			if (results.sum != answers[current_query]) {
				fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
				        answers[current_query], sum);
			}
			double base_time = chrono::duration<double>(end - start).count();
			query_times.q_time[current_query].query_processing += base_time;
			double estimated_time = 0;
			size_t ITERATIONS = 10;
			double estimated_delta = 0.5;
			double offset = estimated_delta / 2;
			for (size_t j = 0; j < ITERATIONS; j++) {
				estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
				                          rangeQueries.rightpredicate[current_query], estimated_delta);
				if (estimated_time > best_convergence_delta_time) {
					estimated_delta -= offset;
				} else {
					estimated_delta += offset;
				}
				offset /= 2;
			}
			start = chrono::system_clock::now();
			results = function(column, rangeQueries.leftpredicate[current_query],
			                   rangeQueries.rightpredicate[current_query], estimated_delta);
			end = chrono::system_clock::now();
			query_times.idx_time[current_query].index_creation +=
			    chrono::duration<double>(end - start).count() - base_time;
			sum = results.sum;
			double time = chrono::duration<double>(end - start).count();
			prefix_sum += chrono::duration<double>(end - start).count();
			query_times.prefix_sum[current_query] += prefix_sum;
			if (sum != answers[current_query]) {
				fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
				        answers[current_query], sum);
			}
			//! now interpolate the real delta
			double cost_per_delta = (time - base_time) / estimated_delta;
			double real_delta = (best_convergence_delta_time - base_time) / cost_per_delta;
			if (column.converged) {
				fprintf(stderr, "Converged on query %lld\n", current_query);
				converged = true;
			}
			deltas[current_query] += real_delta;
			start = chrono::system_clock::now();
			results = function(column, rangeQueries.leftpredicate[current_query + 1],
			                   rangeQueries.rightpredicate[current_query + 1], 0);
			end = chrono::system_clock::now();
			if (results.sum != answers[current_query + 1]) {
				fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
				        answers[current_query + 1], sum);
			}
			full_scan_time = chrono::duration<double>(end - start).count();
		} else {

			double estimated_time = 0;
			size_t ITERATIONS = 10;
			double estimated_delta = 0.5;
			double offset = estimated_delta / 2;
			if (FIXED_BUDGET) {
				estimated_delta = 0.25;
				estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
				                          rangeQueries.rightpredicate[current_query], estimated_delta);
			} else {
				for (size_t j = 0; j < ITERATIONS; j++) {
					estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
					                          rangeQueries.rightpredicate[current_query], estimated_delta);
					if (estimated_time > INTERACTIVITY_THRESHOLD) {
						estimated_delta -= offset;
					} else {
						estimated_delta += offset;
					}
					offset /= 2;
				}
			}

			if (estimated_time < INTERACTIVITY_THRESHOLD / 2) {
				//! first get the base time with delta = 0
				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], 0);
				end = chrono::system_clock::now();
				if (results.sum != answers[current_query]) {
					fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
					        answers[current_query], sum);
				}
				double base_time = chrono::duration<double>(end - start).count();
				query_times.q_time[current_query].query_processing += base_time;

				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], estimated_delta);
				end = chrono::system_clock::now();
				query_times.idx_time[current_query].index_creation +=
				    chrono::duration<double>(end - start).count() - base_time;
				sum = results.sum;
				double time = chrono::duration<double>(end - start).count();
				prefix_sum += chrono::duration<double>(end - start).count();
				query_times.prefix_sum[current_query] += prefix_sum;
				if (sum != answers[current_query]) {
					//!            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got :
					//!            %lld \n", current_query,
					//!                    answers[current_query], res);
					fprintf(stderr, " ");
				}
				//! now interpolate the real delta
				double cost_per_delta = (time - base_time) / estimated_delta;
				double real_delta = (INTERACTIVITY_THRESHOLD - base_time) / cost_per_delta;
				if (column.converged) {
					fprintf(stderr, "Converged on query %lld\n", current_query);
					converged = true;
				}
				deltas[current_query] += real_delta;
				query_times.cost_model[current_query] += estimated_time;
				auto auxTime = time;
				double int_thres_aux = INTERACTIVITY_THRESHOLD - auxTime;
				estimated_delta = 0.5;
				offset = estimated_delta / 2;
				for (size_t j = 0; j < ITERATIONS; j++) {
					estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
					                          rangeQueries.rightpredicate[current_query], estimated_delta);
					if (estimated_time > int_thres_aux) {
						estimated_delta -= offset;
					} else {
						estimated_delta += offset;
					}
					offset /= 2;
				}
				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], 0);
				end = chrono::system_clock::now();
				if (results.sum != answers[current_query]) {
					fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
					        answers[current_query], sum);
				}
				base_time = chrono::duration<double>(end - start).count();
				query_times.q_time[current_query].query_processing += base_time;

				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], estimated_delta);
				end = chrono::system_clock::now();
				query_times.idx_time[current_query].index_creation +=
				    chrono::duration<double>(end - start).count() - base_time;
				fprintf(stderr, "Base Time %f\\n", base_time);
				fprintf(stderr, "IdxC Time %f\\n", chrono::duration<double>(end - start).count() - base_time);

				sum = results.sum;
				time = chrono::duration<double>(end - start).count();
				prefix_sum += chrono::duration<double>(end - start).count();
				query_times.prefix_sum[current_query] += prefix_sum;
				if (sum != answers[current_query]) {
					//!            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got :
					//!            %lld \n", current_query,
					//!                    answers[current_query], res);
					fprintf(stderr, " ");
				}
				//! now interpolate the real delta
				cost_per_delta = (time - base_time) / estimated_delta;
				real_delta = (int_thres_aux - base_time) / cost_per_delta;
				if (column.converged) {
					fprintf(stderr, "Converged on query %lld\n", current_query);
					converged = true;
				}
				fprintf(stderr, "Estimated Time %f\\n", estimated_time);
				fprintf(stderr, "auxTime %f\\n", auxTime);

				deltas[current_query] += real_delta;
				query_times.cost_model[current_query] = estimated_time + auxTime;
			} else {
				//! first get the base time with delta = 0
				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], 0);
				end = chrono::system_clock::now();
				if (results.sum != answers[current_query]) {
					fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
					        answers[current_query], sum);
				}
				double base_time = chrono::duration<double>(end - start).count();
				query_times.q_time[current_query].query_processing += base_time;

				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], estimated_delta);
				end = chrono::system_clock::now();
				query_times.idx_time[current_query].index_creation +=
				    chrono::duration<double>(end - start).count() - base_time;
				sum = results.sum;
				double time = chrono::duration<double>(end - start).count();
				prefix_sum += chrono::duration<double>(end - start).count();
				query_times.prefix_sum[current_query] += prefix_sum;
				if (sum != answers[current_query]) {
					//!            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got :
					//!            %lld \n", current_query,
					//!                    answers[current_query], res);
					fprintf(stderr, " ");
				}
				//! now interpolate the real delta
				double cost_per_delta = (time - base_time) / estimated_delta;
				double real_delta = (INTERACTIVITY_THRESHOLD - base_time) / cost_per_delta;
				if (column.converged) {
					fprintf(stderr, "Converged on query %lld\n", current_query);
					converged = true;
				}
				deltas[current_query] += real_delta;
				query_times.cost_model[current_query] += estimated_time;
			}
		}
	}
	column.Clear();
}

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
