#include "../include/progressive/progressive_indexing.h"

using namespace std;
void progressive_indexing(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &deltas,
                          progressive_function function) {
	chrono::time_point<chrono::system_clock> start, end;
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
		assert(results.sum == answers[current_query]);
		start = chrono::system_clock::now();

		results = function(column, rangeQueries.leftpredicate[current_query],
		                   rangeQueries.rightpredicate[current_query], DELTA);
		end = chrono::system_clock::now();
		prefix_sum += chrono::duration<double>(end - start).count();
		query_times.prefix_sum[current_query] += prefix_sum;
		query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count() - baseline;
		deltas[current_query] += DELTA;
	}
}

void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                     vector<double> &deltas, progressive_function function,
                                     estimate_function estimate) {
	chrono::time_point<chrono::system_clock> start, end;
	int64_t sum = 0;
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
	assert(results.sum == answers[0]);
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
			assert(results.sum == answers[current_query]);
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
			assert(sum == answers[current_query]);
			//! now interpolate the real delta
			double cost_per_delta = (time - base_time) / estimated_delta;
			double real_delta = (best_convergence_delta_time - base_time) / cost_per_delta;
			deltas[current_query] += real_delta;
			start = chrono::system_clock::now();
			results = function(column, rangeQueries.leftpredicate[current_query + 1],
			                   rangeQueries.rightpredicate[current_query + 1], 0);
			end = chrono::system_clock::now();
			assert(results.sum == answers[current_query + 1]);
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
				assert(results.sum == answers[current_query]);
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
				assert(sum == answers[current_query]);
				//! now interpolate the real delta
				double cost_per_delta = (time - base_time) / estimated_delta;
				double real_delta = (INTERACTIVITY_THRESHOLD - base_time) / cost_per_delta;
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
				assert(results.sum == answers[current_query]);
				base_time = chrono::duration<double>(end - start).count();
				query_times.q_time[current_query].query_processing += base_time;

				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], estimated_delta);
				end = chrono::system_clock::now();
				query_times.idx_time[current_query].index_creation +=
				    chrono::duration<double>(end - start).count() - base_time;
				sum = results.sum;
				time = chrono::duration<double>(end - start).count();
				prefix_sum += chrono::duration<double>(end - start).count();
				query_times.prefix_sum[current_query] += prefix_sum;
				assert(sum == answers[current_query]);
				//! now interpolate the real delta
				cost_per_delta = (time - base_time) / estimated_delta;
				real_delta = (int_thres_aux - base_time) / cost_per_delta;
				deltas[current_query] += real_delta;
				query_times.cost_model[current_query] = estimated_time + auxTime;
			} else {
				//! first get the base time with delta = 0
				start = chrono::system_clock::now();
				results = function(column, rangeQueries.leftpredicate[current_query],
				                   rangeQueries.rightpredicate[current_query], 0);
				end = chrono::system_clock::now();
				assert(results.sum == answers[current_query]);
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
				assert(sum == answers[current_query]);
				//! now interpolate the real delta
				double cost_per_delta = (time - base_time) / estimated_delta;
				double real_delta = (INTERACTIVITY_THRESHOLD - base_time) / cost_per_delta;
				deltas[current_query] += real_delta;
				query_times.cost_model[current_query] += estimated_time;
			}
		}
	}
	column.Clear();
}
