#include <vector>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <ctime>
#include <ios>
#include <iomanip>
#include <climits>
#include <cassert>
#include <cmath>
#include <string>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <queue>
#include <math.h>
#include "include/structs.h"
#include "include/util/file_manager.h"
#include "include/util/binary_search.h"
#include "include/full_index/hybrid_radix_insert_sort.h"
#include "include/full_index/bulkloading_bp_tree.h"
#include "include/cracking/standard_cracking.h"
#include "include/cracking/stochastic_cracking.h"
#include "include/cracking/progressive_stochastic_cracking.h"
#include "include/progressive/incremental.h"
#include "include/generate/random.h"
#include "include/cracking/cracking_updates.h"


#pragma clang diagnostic ignored "-Wformat"

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
int64_t COLUMN_SIZE, NUM_QUERIES, L2_SIZE, COLUMN_SIZE_DUMMY;
int ALGORITHM, RUN_CORRECTNESS, NUM_UPDATES, FREQUENCY, RADIXSORT_TOTAL_BYTES;
double ALLOWED_SWAPS_PERCENTAGE;
int64_t num_partitions = 0;
double DELTA, INTERACTIVITY_THRESHOLD;
TotalTime query_times;
size_t current_query;

typedef ResultStruct (*progressive_function)(Column& c, int64_t low, int64_t high, double delta);
typedef double (*estimate_function)(Column &c, int64_t low, int64_t high, double delta);

void full_scan(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        start = chrono::system_clock::now();
        int64_t sum = 0;
        for (size_t j = 0; j < COLUMN_SIZE; j++)
            if (column.data[j] >= rangeQueries.leftpredicate[current_query] && column.data[j] <= rangeQueries.rightpredicate[current_query])
                sum += column.data[j];
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query, answers[current_query], sum);
    }
}



int64_t scanQuery(IndexEntry *c, int64_t from, int64_t to) {
    int64_t sum = 0;
    for (int64_t i = from; i <= to; i++) {
        sum += c[i].m_key;
    }

    return sum;
}

void *fullIndex(IndexEntry *c) {
//    chrono::time_point<chrono::system_clock> start, end;
//    start = chrono::system_clock::now();
    hybrid_radixsort_insert(c, COLUMN_SIZE);
//    end = chrono::system_clock::now();
//    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
//    start = chrono::system_clock::now();
    void *I = build_bptree_bulk(c, COLUMN_SIZE);
//    end = chrono::system_clock::now();
//    query_times.q_time[0].query_processing += chrono::duration<double>(end - start).count();
    return I;
}

void full_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *data = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        data[i].m_key = column.data[i];
        data[i].m_rowId = i;
    }


    BulkBPTree *T = (BulkBPTree *) fullIndex(data);
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        // query
        start = chrono::system_clock::now();
        int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
        int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
//        end = chrono::system_clock::now();
//        query_times.q_time[current_query].index_lookup += chrono::duration<double>(end - start).count();
//        start = chrono::system_clock::now();
        int64_t sum = scanQuery(data, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query, answers[current_query], sum);
    }
    free(data);
    free(T);
}

void standard_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;


    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                             rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        //Querying
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
//        end = chrono::system_clock::now();
//        query_times.q_time[current_query].index_lookup += chrono::duration<double>(end - start).count();
//        start = chrono::system_clock::now();
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query, answers[current_query], sum);
    }
    free(crackercolumn);
}

int64_t queryStochastic(IndexEntry *crackercolumn, int limit) {
    int offset1 = 0;
    int offset2 = limit;
    return scanQuery(crackercolumn, offset1, offset2);

}

void stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;


    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;

        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();

        T = stochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query] + 1, qo);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        start = chrono::system_clock::now();

        //Querying
        if (qo->view1) {
            qo->sum += queryStochastic(qo->view1, qo->view_size1 - 1);
        }
        if (qo->middlePart_size > 0) {
            qo->sum += queryStochastic(qo->middlePart, qo->middlePart_size - 1);
        }
        if (qo->view2) {
            qo->sum += queryStochastic(qo->view2, qo->view_size2 - 1);
        }
        if (qo->view1) {
            free(qo->view1);
            qo->view1 = NULL;
        }
        if (qo->view2) {
            free(qo->view2);
            qo->view2 = NULL;
        }
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();

        if (qo->sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query, answers[current_query], qo->sum);
    }
    free(crackercolumn);
}

void progressive_stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;
        start = chrono::system_clock::now();
        //Partitioning Column and Inserting in Cracker Indexing
        T = progressiveStochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                                          rangeQueries.rightpredicate[current_query] + 1, qo);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        start = chrono::system_clock::now();

        //Querying
        if (qo->view1) {
            qo->sum += queryStochastic(qo->view1, qo->view_size1 - 1);
        }
        if (qo->middlePart_size > 0) {
            qo->sum += queryStochastic(qo->middlePart, qo->middlePart_size - 1);
        }
        if (qo->view2) {
            qo->sum += queryStochastic(qo->view2, qo->view_size2 - 1);
        }
        if (qo->view1) {
            free(qo->view1);
            qo->view1 = NULL;
        }
        if (qo->view2) {
            free(qo->view2);
            qo->view2 = NULL;
        }
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (qo->sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query, answers[current_query], qo->sum);

    }
    free(crackercolumn);

}

void generate_equal_size_partitions_order(vector<int64_t> *partitions, int64_t min, int64_t max) {
    int64_t medium = (max + min) / 2;
    partitions->push_back(medium);
    num_partitions++;
    if (num_partitions > 999)
        return;
    generate_equal_size_partitions_order(partitions, min, medium);
    generate_equal_size_partitions_order(partitions, medium, max);

}


void coarse_granular_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, middle, end;
    chrono::duration<double> elapsed_seconds;
    start = chrono::system_clock::now();
    vector<int64_t> partitions;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
//    end = chrono::system_clock::now();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    // Running 1000 - equal sized partitions
    generate_equal_size_partitions_order(&partitions, 0, COLUMN_SIZE);
    for (size_t i = 0; i < 1000; i++) {
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, partitions[i], partitions[i]);
    }

    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();

    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        // crack
        start = chrono::system_clock::now();

        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                             rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();

        // query
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int64_t offset1 = p1->first;
        int64_t offset2 = p2->second;
        free(p1);
        free(p2);
//        end = chrono::system_clock::now();
//        query_times.q_time[current_query].index_lookup += chrono::duration<double>(end - start).count();
//        start = chrono::system_clock::now();
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query, answers[current_query], sum);
    }
    free(crackercolumn);
}

int64_t scanQuery(int64_t *c, int64_t from, int64_t to) {
    int64_t sum = 0;
    for (int64_t i = from; i <= to; i++) {
        sum += c[i];
    }

    return sum;
}

void *fullIndex(int64_t *c, int n) {
    void *I = build_bptree_bulk_int(c, n);
    return I;
}

void progressive_indexing(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                           vector<double> &deltas, progressive_function function) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    if (function == range_query_incremental_bucketsort_equiheight) {
        // create the initial buckets for bucketsort without counting the time
        // we do this because it throws off our benchmarking otherwise
        // otherwise we spent one iteration on determining min,max of the column
        // which is unrelated to the delta of the algorithm or anything else
        // we assume the database knows these from statistics in the paper
        // so we just run a "warm up run" to determine these here prior to benchmarking
        function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
    }

    if (function == range_query_incremental_radixsort_msd ||
        function == range_query_incremental_bucketsort_equiheight) {
        // initialize the sort index
        column.sortindex.resize(column.data.size());
        column.bucket_index.final_index = new int64_t[column.data.size()];
        column.bucket_index.final_index_entries = 0;
    }

    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        ResultStruct results;



        if (column.converged) {
            start = chrono::system_clock::now();
            int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
            int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
            sum = scanQuery(column.final_data, offset1, offset2);
            end = chrono::system_clock::now();
            query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        } else {
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query], 0);
            end = chrono::system_clock::now();
            double baseline = chrono::duration<double>(end - start).count();
            query_times.q_time[current_query].query_processing += baseline;
            if (results.sum != answers[current_query])
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query, answers[current_query], sum);
            start = chrono::system_clock::now();

            results = function(column, rangeQueries.leftpredicate[current_query],
                                                        rangeQueries.rightpredicate[current_query], DELTA);
            end = chrono::system_clock::now();

            sum = results.sum;
            query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count() - baseline;
        }

        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query, answers[current_query], sum);
        if (!converged && column.converged) {
            converged = true;
            T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());

        }
        deltas[current_query] += DELTA;
    }
}


void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                       vector<double> &deltas,progressive_function function
                                      , estimate_function estimate) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    if (function == range_query_incremental_bucketsort_equiheight) {
        // create the initial buckets for bucketsort without counting the time
        // we do this because it throws off our benchmarking otherwise
        // otherwise we spent one iteration on determining min,max of the column
        // which is unrelated to the delta of the algorithm or anything else
        // we assume the database knows these from statistics in the paper
        // so we just run a "warm up run" to determine these here prior to benchmarking
        function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
    }

    if (function == range_query_incremental_radixsort_msd ||
        function == range_query_incremental_bucketsort_equiheight) {
        // initialize the sort index
        column.sortindex.resize(column.data.size());
        column.bucket_index.final_index = new int64_t[column.data.size()];
        column.bucket_index.final_index_entries = 0;
    }
    // 0.5s
    double interactivity_threshold = INTERACTIVITY_THRESHOLD;
    // Run Dummy Full Scan to check if its higher or lower than the interactivity threshold
    double full_scan_time;
    start = chrono::system_clock::now();
    for (size_t j = 0; j < COLUMN_SIZE; j++)
        if (column.data[j] >= rangeQueries.leftpredicate[0] && column.data[j] <= rangeQueries.rightpredicate[0])
            sum += column.data[j];
    end = chrono::system_clock::now();
    full_scan_time = chrono::duration<double>(end - start).count();
    if (sum != answers[0])
        fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", 0, answers[0], sum);
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        ResultStruct results;
        if (full_scan_time > interactivity_threshold) {
            double best_convergence_delta = 0.22; // FIXME: This is algorithm dependent

            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                                                        rangeQueries.rightpredicate[current_query], best_convergence_delta);
            sum = results.sum;
            if (sum != answers[current_query]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query, answers[current_query], sum);
            }
            end = chrono::system_clock::now();
            full_scan_time = chrono::duration<double>(end - start).count();
            interactivity_threshold = full_scan_time;
            deltas[current_query] += best_convergence_delta;
        } else {
            double estimated_time = 0;
            size_t ITERATIONS = 5;
            double estimated_delta = 0.5;
            double offset = estimated_delta / 2;
            for (size_t j = 0; j < ITERATIONS; j++) {
                estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
                                                              rangeQueries.rightpredicate[current_query], estimated_delta);
                if (estimated_time > interactivity_threshold) {
                    estimated_delta -= offset;
                } else {
                    estimated_delta += offset;
                }
                offset /= 2;
            }

            // first get the base time with delta = 0
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                                                        rangeQueries.rightpredicate[current_query], 0);
            end = chrono::system_clock::now();

            double base_time = chrono::duration<double>(end - start).count();
            query_times.q_time[current_query].query_processing += base_time;

            // now get the time with delta = estimated_delta
            if (converged) {
                start = chrono::system_clock::now();
                int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
                int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
                sum = scanQuery(column.final_data, offset1, offset2);
                end = chrono::system_clock::now();
                query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
            } else {
                start = chrono::system_clock::now();
                results = function(column, rangeQueries.leftpredicate[current_query],
                                                            rangeQueries.rightpredicate[current_query], estimated_delta);
                end = chrono::system_clock::now();
                query_times.idx_time[current_query].index_creation+= chrono::duration<double>(end - start).count() - base_time;


                sum = results.sum;
            }
            end = chrono::system_clock::now();
            double time = chrono::duration<double>(end - start).count();
            if (sum != answers[current_query]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query, answers[current_query], sum);
            }
            // now interpolate the real delta
            double cost_per_delta = (time - base_time) / estimated_delta;
            double real_delta = (interactivity_threshold - base_time) / cost_per_delta;
            if (!converged && column.converged) {
                converged = true;
                T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
            }
            if (!converged)
                deltas[current_query] += real_delta;
        }
    }
    column.Clear();
}

void generate_updates(vector<int64_t> &updates) {
    Random r(140384);
    for (int i = 0; i < FREQUENCY * NUM_UPDATES; i++) {
        updates[i] = abs(r.nextInt());
    }
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
    fprintf(stderr, "   --progressive-cracking-swap\n");
    fprintf(stderr, "   --delta\n");
    fprintf(stderr, "   --correctness\n");
    fprintf(stderr, "   --freq-updates\n");
    fprintf(stderr, "   --num-updates\n");
    fprintf(stderr, "   --interactivity-threshold\n");

}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {
    COLUMN_FILE_PATH = "generated_data/10000000/column";
    QUERIES_FILE_PATH = "generated_data/10000000/query_1_2";
    ANSWER_FILE_PATH = "generated_data/10000000/answer_1_2";
    L2_SIZE = 16000; // 320000 64 bit integers
    ALLOWED_SWAPS_PERCENTAGE = 0.1;
    NUM_QUERIES = 10000;
    COLUMN_SIZE = 10000000;
    ALGORITHM = 0;
    DELTA = 0.1;
    RUN_CORRECTNESS = 0; // By Default run regular runs
    NUM_UPDATES = 1000;
    FREQUENCY = 10;
    INTERACTIVITY_THRESHOLD = 0.5;

    int repetition = 1;

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
        } else if (arg_name == "progressive-cracking-swap") {
            ALLOWED_SWAPS_PERCENTAGE = atof(arg_value.c_str());
        } else if (arg_name == "delta") {
            DELTA = atof(arg_value.c_str());
        } else if (arg_name == "correctness") {
            RUN_CORRECTNESS = atoi(arg_value.c_str());
        } else if (arg_name == "num-updates") {
            NUM_UPDATES = atoi(arg_value.c_str());
        } else if (arg_name == "freq-updates") {
            FREQUENCY = atoi(arg_value.c_str());
        } else if (arg_name == "interactivity-threshold") {
            FREQUENCY = atoi(arg_value.c_str());
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

    if (!RUN_CORRECTNESS)
        repetition = 1;
    query_times.Initialize(NUM_QUERIES);
    vector<double> times(NUM_QUERIES);
    vector<double> deltas(NUM_QUERIES);
    current_query = 0;
    for (size_t i = 0; i < repetition; i++) {
        switch (ALGORITHM) {
            case 1:
                full_scan(c, rangequeries, answers);
                break;
            case 2:
                full_index(c, rangequeries, answers);
                break;
            case 3:
                standard_cracking(c, rangequeries, answers);
                break;
            case 4:
                stochastic_cracking(c, rangequeries, answers);
                break;
            case 5:
                progressive_stochastic_cracking(c, rangequeries, answers);
                break;
            case 6:
                coarse_granular_index(c, rangequeries, answers);
                break;
            case 7:
                progressive_indexing(c, rangequeries, answers, deltas,range_query_incremental_quicksort);
                break;
            case 8:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                        range_query_incremental_quicksort,get_estimated_time_quicksort);
                break;
            case 9:
                progressive_indexing(c, rangequeries, answers, deltas,range_query_incremental_bucketsort_equiheight);
                break;
            case 10:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_bucketsort_equiheight,get_estimated_time_bucketsort);
                break;
            case 11:
                progressive_indexing(c, rangequeries, answers, deltas,range_query_incremental_radixsort_lsd);
                break;
            case 12:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_radixsort_lsd,get_estimated_time_radixsort_lsd);
                break;
            case 13:
                progressive_indexing(c, rangequeries, answers, deltas,range_query_incremental_radixsort_msd);
                break;
            case 14:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_radixsort_msd,get_estimated_time_radixsort_msd);
                break;
        }
    }
    if (!RUN_CORRECTNESS)
        for (size_t i = 0; i < NUM_QUERIES; i++) {
            double total_time, total_indexing,total_querying;
//            total_indexing = query_times.idx_time[i].index_lookup + query_times.idx_time[i].sort + query_times.idx_time[i].index_update + query_times.idx_time[i].data_copy;
//            total_querying = query_times.q_time[i].index_lookup + query_times.q_time[i].filtering;
            total_time = query_times.idx_time[i].index_creation + query_times.q_time[i].query_processing;
            cout << deltas[i] / repetition << ";"  << query_times.q_time[i].query_processing / repetition << ";"  << query_times.idx_time[i].index_creation / repetition <<  ";" << total_time / repetition  << "\n";
        }
}
