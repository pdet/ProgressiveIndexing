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

void progressive_quicksort(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<double> &time,
                           vector<double> &deltas) {
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
            results = range_query_incremental_quicksort(column, rangeQueries.leftpredicate[i],
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

void progressive_quicksort_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                      vector<double> &times, vector<double> &deltas) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    // The target interactivity threshold, scale for data size
    double interactivity_threshold = 0.5 * (column.data.size() / 100000000.0);

    double estimated_time = 0;
    for (int i = 0; i < NUM_QUERIES; i++) {
        size_t ITERATIONS = 5;
        double estimated_delta = 0.5;
        double offset = estimated_delta / 2;
        for (size_t j = 0; j < ITERATIONS; j++) {
            estimated_time = get_estimated_time_quicksort(column, rangeQueries.leftpredicate[i],
                                                          rangeQueries.rightpredicate[i], estimated_delta);
            if (estimated_time > interactivity_threshold) {
                estimated_delta -= offset;
            } else {
                estimated_delta += offset;
            }
            offset /= 2;
        }
        ResultStruct results;
        // first get the base time with delta = 0
        start = chrono::system_clock::now();
        results = range_query_incremental_quicksort(column, rangeQueries.leftpredicate[i],
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
            results = range_query_incremental_quicksort(column, rangeQueries.leftpredicate[i],
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

        times[i] += chrono::duration<double>(end - start).count();
        if (!converged && column.converged) {
            converged = true;
            T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
        }
        if (!converged)
            deltas[i] += real_delta;
    }
    column.Clear();
}

void generate_updates(vector<int64_t> &updates) {
    Random r(140384);
    for (int i = 0; i < FREQUENCY * NUM_UPDATES; i++) {
        updates[i] = abs(r.nextInt());
    }
}

void cracking_merge_complete(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                             vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            sort(begin(updates->data), std::end(updates->data));
            merge(crackercolumn, capacity, T, *updates, 0, updates->data.size() - 1);
            updates->data.clear();
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void cracking_merge_gradually(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                              vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        bool updates_is_sorted = false;
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        if (updates->data.size()) {
            int64_t initial_offset = 0;
            int64_t final_offset = 0;
            if (!updates_is_sorted) {
                sort(begin(updates->data), std::end(updates->data));
                updates_is_sorted = true;
            }
            initial_offset = binary_search_gte(&updates->data[0], rangeQueries.leftpredicate[i], 0,
                                               updates->data.size() - 1);
            if (initial_offset > 0)
                initial_offset--;
            final_offset = binary_search_gte(&updates->data[0], rangeQueries.rightpredicate[i], 0,
                                             updates->data.size() - 1);
            if (final_offset < updates->data.size() - 1)
                final_offset++;
            merge(crackercolumn, capacity, T, *updates, initial_offset, final_offset);
            updates->data.erase(updates->data.begin() + initial_offset, updates->data.begin() + final_offset + 1);

        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void cracking_merge_ripple(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                           vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        bool updates_is_sorted = false;
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        if (updates->data.size()) {
            int64_t initial_offset = 0;
            int64_t final_offset = 0;
            sort(begin(updates->data), std::end(updates->data));
            initial_offset = binary_search_gte(&updates->data[0], rangeQueries.leftpredicate[i], 0,
                                               updates->data.size() - 1);
            final_offset = binary_search_gte(&updates->data[0], rangeQueries.rightpredicate[i], 0,
                                             updates->data.size() - 1);
            if (initial_offset < final_offset) {
                while (final_offset < updates->data.size() &&
                       updates->data[final_offset] <= rangeQueries.rightpredicate[i]) {
                    final_offset++;
                }
                merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset - 1,
                             rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
            } else if (final_offset == updates->data.size() - 1 && initial_offset == updates->data.size() - 1
                       && updates->data[final_offset] >= rangeQueries.leftpredicate[i] &&
                       updates->data[final_offset] <= rangeQueries.rightpredicate[i])
                merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset,
                             rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);

        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void
progressive_mergesort(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                      vector<double> &times) {
    chrono::time_point<chrono::system_clock> start, end;
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    bool converged = false;
    Column *updates = new Column();
    std::vector<Column *> sort_chunks;
    Column *merge_column = nullptr;
    size_t left_column = 0, right_column = 0, merge_index = 0;
    ssize_t left_chunk, right_chunk;

    std::vector<bool> has_converged;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;
    int unsorted_column_count = 1;
    double SORTED_COLUMN_RATIO = 16;

    for (int i = 0; i < NUM_QUERIES; i++) {
        // Time to fill append list
        start = chrono::system_clock::now();
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            DELTA = 0.05;
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        ResultStruct results;

        double original_delta = unsorted_column_count == 0 ? 0 : DELTA / unsorted_column_count;
        results = range_query_incremental_quicksort(column, rangeQueries.leftpredicate[i],
                                                    rangeQueries.rightpredicate[i], original_delta);
        if (!converged && column.converged) {
            converged = true;
            unsorted_column_count--;
        }
        for (size_t j = 0; j < sort_chunks.size(); j++) {
            auto &chunk = sort_chunks[j];
            double sort_chunk_delta = original_delta * ((double) column.data.size() / (double) chunk->data.size());
            results.merge(range_query_incremental_quicksort(*chunk, rangeQueries.leftpredicate[i],
                                                            rangeQueries.rightpredicate[i], sort_chunk_delta));
            if (!has_converged[j] && chunk->converged) {
                has_converged[j] = true;
                unsorted_column_count--;
            }
        }
        for (auto &element : updates->data) {
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            results.maybe_push_back(element, matching);
        }

        if (!merge_column && unsorted_column_count == 0 && sort_chunks.size() > 1) {
            // start a merge
            merge_column = new Column();
            left_chunk = sort_chunks.size() - 2;
            right_chunk = sort_chunks.size() - 1;
            merge_column->data.resize(sort_chunks[left_chunk]->data.size() + sort_chunks[right_chunk]->data.size());
            left_column = 0;
            right_column = 0;
            merge_index = 0;
        }
        if (merge_column) {
            ssize_t todo_merge = std::min((ssize_t) merge_column->data.size() - merge_index,
                                          (ssize_t) DELTA * column.data.size());
            for (size_t j = 0; j < todo_merge; j++) {
                if (left_column < sort_chunks[left_chunk]->data.size() &&
                    (right_column >= sort_chunks[right_chunk]->data.size() ||
                     sort_chunks[left_chunk]->data[left_column] < sort_chunks[right_chunk]->data[right_column])) {
                    merge_column->data[merge_index++] = sort_chunks[left_chunk]->data[left_column++];
                } else {
                    merge_column->data[merge_index++] = sort_chunks[right_chunk]->data[right_column++];
                }
            }
            if (merge_index == merge_column->data.size()) {
                // finish merging
                delete sort_chunks[left_chunk];
                delete sort_chunks[right_chunk];
                sort_chunks.erase(sort_chunks.begin() + left_chunk, sort_chunks.begin() + right_chunk + 1);
                sort_chunks.insert(sort_chunks.begin(), merge_column);
                merge_column = nullptr;
            }
        }

        end = chrono::system_clock::now();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (results.sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    results.sum);
        }

        times[i] += chrono::duration<double>(end - start).count();
        if (updates->data.size() > column.data.size() / SORTED_COLUMN_RATIO) {
            // start a quick sort on the updates
            sort_chunks.push_back(updates);
            has_converged.push_back(false);
            updates = new Column();
            unsorted_column_count++;
        }
    }
    updates->Clear();

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

    int total;

    RangeQuery rangequeries;
    load_queries(&rangequeries, QUERIES_FILE_PATH, NUM_QUERIES);
    Column c;
    c.data = vector<int64_t>(COLUMN_SIZE);
    load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);

    vector<int64_t> answers;
    load_answers(&answers, ANSWER_FILE_PATH, NUM_QUERIES);

    if (!RUN_CORRECTNESS)
        repetition = 5;

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
                progressive_quicksort(c, rangequeries, answers, times, deltas);
                break;
            case 7:
                progressive_quicksort_cost_model(c, rangequeries, answers, times, deltas);
                break;
                // In the following cases the algorithms are executed with Updates
            case 8:
                cracking_merge_complete(c, rangequeries, answers, times);
                break;
            case 9:
                cracking_merge_gradually(c, rangequeries, answers, times);
                break;
            case 10:
                cracking_merge_ripple(c, rangequeries, answers, times);
                break;
            case 11:
                progressive_mergesort(c, rangequeries, answers, times);
                break;
        }
    }
    if (!RUN_CORRECTNESS)
        for (size_t i = 0; i < NUM_QUERIES; i++) {
            cout << deltas[i] / repetition << ";" << times[i] / repetition << "\n";
        }
}
