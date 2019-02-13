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
int ALGORITHM, RUN_CORRECTNESS, NUM_UPDATES, FREQUENCY;
double ALLOWED_SWAPS_PERCENTAGE;
int64_t num_partitions = 0;
double DELTA;


typedef ResultStruct (*progressive_function)(Column& c, int64_t low, int64_t high, double delta);
typedef double (*estimate_function)(Column &c, int64_t low, int64_t high, double delta);

void full_scan(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, end;
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        int64_t sum = 0;
        for (size_t j = 0; j < COLUMN_SIZE; j++)
            if (column.data[j] >= rangeQueries.leftpredicate[i] && column.data[j] <= rangeQueries.rightpredicate[i])
                sum += column.data[j];
        end = chrono::system_clock::now();
        time[i] += chrono::duration<double>(end - start).count();
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", i, answers[i], sum);
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
    hybrid_radixsort_insert(c, COLUMN_SIZE);
    void *I = build_bptree_bulk(c, COLUMN_SIZE);

    return I;
}

void full_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *data = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        data[i].m_key = column.data[i];
        data[i].m_rowId = i;
    }
    BulkBPTree *T = (BulkBPTree *) fullIndex(data);
    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();
    for (int i = 0; i < NUM_QUERIES; i++) {
        // query
        start = chrono::system_clock::now();
        int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[i]);
        int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[i]);
        int64_t sum = scanQuery(data, offset1, offset2);
        end = chrono::system_clock::now();
        time[i] += chrono::duration<double>(end - start).count();
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i], sum);

    }
    free(data);
    free(T);
}

void standard_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();

    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        //Partitioning Column and Inserting in Cracker Indexing
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        time[i] += chrono::duration<double>(end - start).count();
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", i, answers[i], sum);
    }
    free(crackercolumn);
}

int64_t queryStochastic(IndexEntry *crackercolumn, int limit) {
    int offset1 = 0;
    int offset2 = limit;
    return scanQuery(crackercolumn, offset1, offset2);

}

void stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;
    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();

    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;


        //Partitioning Column and Inserting in Cracker Indexing
        T = stochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[i],
                               rangeQueries.rightpredicate[i] + 1, qo);

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
        time[i] += chrono::duration<double>(end - start).count();
        if (qo->sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", i, answers[i], qo->sum);
    }
    free(crackercolumn);
}

void progressive_stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                     vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;
    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;
        //Partitioning Column and Inserting in Cracker Indexing
        T = progressiveStochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[i],
                                          rangeQueries.rightpredicate[i] + 1, qo);

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
        time[i] += chrono::duration<double>(end - start).count();
        if (qo->sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", i, answers[i], qo->sum);

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


void coarse_granular_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time) {
    chrono::time_point<chrono::system_clock> start, middle, end;
    chrono::duration<double> elapsed_seconds;
    start = chrono::system_clock::now();
    vector<int64_t> partitions;
    generate_equal_size_partitions_order(&partitions, 0, COLUMN_SIZE);
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    // Running 1000 - equal sized partitions
    for (size_t i = 0; i < 1000; i++) {
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, partitions[i], partitions[i]);
    }

    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();

    for (size_t q = 0; q < NUM_QUERIES; q++) {
        start = chrono::system_clock::now();
        // crack
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[q],
                             rangeQueries.rightpredicate[q] + 1);
        // query
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[q], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[q] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int64_t offset1 = p1->first;
        int64_t offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);

        end = chrono::system_clock::now();
        time[q] += chrono::duration<double>(end - start).count();
        if (sum != answers[q])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", q, answers[q], sum);
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

void progressive_indexing(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time,
                           vector<double> &deltas, progressive_function function) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    for (int i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        ResultStruct results;
        if (column.converged) {
            int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[i]);
            int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[i]);
            sum = scanQuery(column.final_data, offset1, offset2);
        } else {
            results = function(column, rangeQueries.leftpredicate[i],
                                                        rangeQueries.rightpredicate[i], DELTA);
            sum = results.sum;
        }

        end = chrono::system_clock::now();
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i], sum);
        time[i] += chrono::duration<double>(end - start).count();
        if (!converged && column.converged) {
            converged = true;
//            cout << "CONVERGED:" << i << "\n";
            T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
        }
        deltas[i] += DELTA;
    }
}


void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                      vector<double> &times, vector<double> &deltas,progressive_function function
                                      , estimate_function estimate) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    // 0.5s
    double interactivity_threshold = 0.5;
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
    for (int i = 0; i < NUM_QUERIES; i++) {
        ResultStruct results;
        if (full_scan_time > interactivity_threshold) {
            double best_convergence_delta = 0.22;

            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[i],
                                                        rangeQueries.rightpredicate[i], best_convergence_delta);
            sum = results.sum;
            if (sum != answers[i]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i], sum);
            }
            end = chrono::system_clock::now();
            times[i] += chrono::duration<double>(end - start).count();
            full_scan_time = times[i];
            interactivity_threshold = full_scan_time;
            deltas[i] += best_convergence_delta;
        } else {
            double estimated_time = 0;
            size_t ITERATIONS = 5;
            double estimated_delta = 0.5;
            double offset = estimated_delta / 2;
            for (size_t j = 0; j < ITERATIONS; j++) {
                estimated_time = estimate(column, rangeQueries.leftpredicate[i],
                                                              rangeQueries.rightpredicate[i], estimated_delta);
                if (estimated_time > interactivity_threshold) {
                    estimated_delta -= offset;
                } else {
                    estimated_delta += offset;
                }
                offset /= 2;
            }

            // first get the base time with delta = 0
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[i],
                                                        rangeQueries.rightpredicate[i], 0);
            end = chrono::system_clock::now();

            double base_time = chrono::duration<double>(end - start).count();

            // now get the time with delta = estimated_delta
            start = chrono::system_clock::now();
            if (converged) {
                int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[i]);
                int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[i]);
                sum = scanQuery(column.final_data, offset1, offset2);
            } else {
                results = function(column, rangeQueries.leftpredicate[i],
                                                            rangeQueries.rightpredicate[i], estimated_delta);
                sum = results.sum;
            }
            end = chrono::system_clock::now();
            double time = chrono::duration<double>(end - start).count();
            if (sum != answers[i]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i], sum);
            }
            // now interpolate the real delta
            double cost_per_delta = (time - base_time) / estimated_delta;
            double real_delta = (interactivity_threshold - base_time) / cost_per_delta;
//            times[i] += real_time;
            times[i] += chrono::duration<double>(end - start).count();
            if (!converged && column.converged) {
                converged = true;
                T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
            }
            if (!converged)
                deltas[i] += real_delta;
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
}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {
    COLUMN_FILE_PATH = "column";
    QUERIES_FILE_PATH = "query";
    ANSWER_FILE_PATH = "answer";
    L2_SIZE = 16000; // 320000 64 bit integers
    ALLOWED_SWAPS_PERCENTAGE = 0.1;
    NUM_QUERIES = 1000;
    COLUMN_SIZE = 10000000;
    ALGORITHM = 0;
    DELTA = 0.1;
    RUN_CORRECTNESS = 0; // By Default run regular runs
    NUM_UPDATES = 1000;
    FREQUENCY = 10;

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
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    RangeQuery rangequeries;
    load_queries(&rangequeries, QUERIES_FILE_PATH, NUM_QUERIES);
    Column c;
    load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);

    vector<int64_t> answers;
    load_answers(&answers, ANSWER_FILE_PATH, NUM_QUERIES);

    if (!RUN_CORRECTNESS)
        repetition = 1;

    vector<double> times(NUM_QUERIES);
    vector<double> deltas(NUM_QUERIES);

    for (size_t i = 0; i < repetition; i++) {
        switch (ALGORITHM) {
            case 0:
                full_scan(c, rangequeries, answers, times);
                break;
            case 1:
                full_index(c, rangequeries, answers, times);
                break;
            case 2:
                standard_cracking(c, rangequeries, answers, times);
                break;
            case 3:
                stochastic_cracking(c, rangequeries, answers, times);
                break;
            case 4:
                progressive_stochastic_cracking(c, rangequeries, answers, times);
                break;
            case 5:
                coarse_granular_index(c, rangequeries, answers, times);
                break;
            case 6:
                progressive_indexing(c, rangequeries, answers, times, deltas,range_query_incremental_quicksort);
                break;
            case 7:
                progressive_indexing_cost_model(c, rangequeries, answers, times, deltas,
                        range_query_incremental_quicksort,get_estimated_time_quicksort);
                break;
            case 8:
                progressive_indexing(c, rangequeries, answers, times, deltas,range_query_incremental_bucketsort_equiheight);
                break;
            case 9:
                progressive_indexing_cost_model(c, rangequeries, answers, times, deltas,
                                                range_query_incremental_bucketsort_equiheight,get_estimated_time_bucketsort);
                break;
            case 10:
                progressive_indexing(c, rangequeries, answers, times, deltas,range_query_incremental_radixsort_lsd);
                break;
            case 11:
                progressive_indexing_cost_model(c, rangequeries, answers, times, deltas,
                                                range_query_incremental_radixsort_lsd,get_estimated_time_radixsort_lsd);
                break;
            case 12:
                progressive_indexing(c, rangequeries, answers, times, deltas,range_query_incremental_radixsort_msd);
                break;
            case 13:
                progressive_indexing_cost_model(c, rangequeries, answers, times, deltas,
                                                range_query_incremental_radixsort_msd,get_estimated_time_radixsort_msd);
                break;
        }
    }
    if (!RUN_CORRECTNESS)
        for (size_t i = 0; i < NUM_QUERIES; i++) {
            cout << deltas[i] / repetition << ";" << times[i] / repetition << "\n";
        }
}
