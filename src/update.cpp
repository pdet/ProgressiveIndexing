
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

#include "include/progressive/incremental.h"
#include "include/util/file_manager.h"
#include "include/util/avl_tree.h"
#include "include/cracking/standard_cracking.h"
#include "include/util/binary_search.h"

#pragma clang diagnostic ignored "-Wformat"

using namespace std;

const int PROGRESSIVE_UPDATE = 100;
const int CRACKING_MERGE_COMPLETE = 101;
const int CRACKING_MERGE_GRADUALLY = 102;
const int CRACKING_MERGE_RIPPLE = 103;

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH, UPDATES_FILE_PATH;
int COLUMN_SIZE, NUM_QUERIES, NUM_UPDATES, INDEXING_TYPE, FREQUENCY,START_UPDATES_AFTER;
double DELTA;

void merge(IndexEntry *&column, size_t &capacity, AvlTree T, Column &updates, int64_t posL, int64_t posH, int64_t _next = -1);

void
progressive_update(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<int64_t> &to_do_updates,
                   vector<double> &progressiveTime, range_query_function function) {
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
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
        int64_t low = min(rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
        int64_t high = max(rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
        start = chrono::system_clock::now();
        if (((i + 1) % FREQUENCY) == 0 && i > START_UPDATES_AFTER) {
            DELTA = 0.05;
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        ResultStruct results;

        double original_delta = unsorted_column_count == 0 ? 0 : DELTA / unsorted_column_count;
        results = function(column, low, high, original_delta);
        if (!converged && column.converged) {
            converged = true;
            unsorted_column_count--;
        }
        for (size_t j = 0; j < sort_chunks.size(); j++) {
            auto &chunk = sort_chunks[j];
            double sort_chunk_delta = original_delta * ((double) column.data.size() / (double) chunk->data.size());
            results.merge(function(*chunk, low, high, sort_chunk_delta));
            if (!has_converged[j] && chunk->converged) {
                has_converged[j] = true;
                unsorted_column_count--;
            }
        }
        for (auto &element : updates->data) {
            int matching = element >= low && element <= high;
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
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= low && element <= high;
            answers[i] += element * matching;
        }

        if (results.sum != answers[i]) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i],
                    results.sum);
        }

        progressiveTime.at(i) += chrono::duration<double>(end - start).count();
        if (updates->data.size() > column.data.size() / SORTED_COLUMN_RATIO) {
            // start a quick sort on the updates
            sort_chunks.push_back(updates);
            has_converged.push_back(false);
            updates = new Column();
            unsorted_column_count++;
        }
    }

    for (int i = 0; i < NUM_QUERIES; i++) {
        cout << progressiveTime[i] << "\n";
    }
}


void insert_preserving_order(vector<int64_t> &cont, int64_t value) {
    vector<int64_t>::iterator it = upper_bound(cont.begin(), cont.end(), value,
                                               std::greater<int>()); // find proper position in descending order
    cont.insert(it, value); // insert before iterator it
}


void cracking_update(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers, vector<int64_t> &to_do_updates,
                     vector<double> &standardcrackingtime) {
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();

    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    standardcrackingtime.at(0) += chrono::duration<double>(end - start).count();

    bool updates_is_sorted = false;
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        int64_t low = min(rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
        int64_t high = max(rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
        start = chrono::system_clock::now();
        // Time to fill append list
        if (((i + 1) % FREQUENCY) == 0 && i > START_UPDATES_AFTER) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            updates_is_sorted = false;
            switch (INDEXING_TYPE) {
                case CRACKING_MERGE_COMPLETE:
                    sort(begin(updates->data), std::end(updates->data));
                    merge(crackercolumn, capacity, T, *updates, 0, updates->data.size() - 1);
                    updates->data.clear();
                    break;
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        if(updates->data.size()){
            int64_t initial_offset = 0;
            int64_t final_offset = 0;
            switch (INDEXING_TYPE) {
                case CRACKING_MERGE_GRADUALLY:
                    if (!updates_is_sorted) {
                        sort(begin(updates->data), std::end(updates->data));
                        updates_is_sorted = true;
                    }
                    initial_offset = binary_search_gte(&updates->data[0],low,0,updates->data.size()-1);
                    if (initial_offset > 0)
                        initial_offset--;
                    final_offset = binary_search_gte(&updates->data[0],high,0,updates->data.size()-1);
                    if (final_offset < updates->data.size()-1)
                        final_offset++;
                    merge(crackercolumn, capacity, T, *updates, initial_offset, final_offset);
                    updates->data.erase(updates->data.begin()+initial_offset,updates->data.begin()+final_offset+1);
                    break;
                case CRACKING_MERGE_RIPPLE:
                    sort(begin(updates->data), std::end(updates->data));
                    initial_offset = binary_search_gte(&updates->data[0],low,0,updates->data.size()-1);
                    final_offset = binary_search_gte(&updates->data[0],high,0,updates->data.size()-1);
                    if (initial_offset < final_offset) {
                        while(final_offset < updates->data.size() && updates->data[final_offset] <= high) {
                            final_offset++;
                        }
                        merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset - 1, low, high);
                    }
                    else if (final_offset == updates->data.size()-1 && initial_offset == updates->data.size()-1
                             && updates->data[final_offset] >= low && updates->data[final_offset] <= high)
                        merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset, low, high);

                    break;
            }
        }
//        89644406
//        99863593
        // if (T)
//        verify_tree(crackercolumn, T);
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, low, high + 1);
//        verify_tree(crackercolumn, T);


        //Querying
        IntPair p1 = FindNeighborsGTE(low, (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(high + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        standardcrackingtime.at(i) += chrono::duration<double>(end - start).count();

        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= low && element <= high;
            answers[i] += element * matching;
        }

//        fprintf(stderr, "Column Size %lld\n  \n", COLUMN_SIZE);
        fprintf(stderr, "Query %lld\n  \n", i);
//        check_column(crackercolumn);
//        check_updates(crackercolumn);
        if (sum != answers[i]) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, answers[i], sum);
        }
    }
    free(crackercolumn);
    for (int i = 0; i < NUM_QUERIES; i++) {
        cout << standardcrackingtime[i] << "\n";
    }
}

void print_help(int argc, char **argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --freq-updates\n");
    fprintf(stderr, "   --num-updates\n");
    fprintf(stderr, "   --column-size\n");
    fprintf(stderr, "   --indexing-type\n");
    fprintf(stderr, "   --delta\n");
}


pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {


    COLUMN_FILE_PATH = "column";
    QUERIES_FILE_PATH = "query";
    ANSWER_FILE_PATH = "answer";
    UPDATES_FILE_PATH = "updates";

    NUM_QUERIES = 1000;
    COLUMN_SIZE = 10000000;
    INDEXING_TYPE = 100;
    DELTA = 0.1;
    NUM_UPDATES = 1000;
    FREQUENCY = 10;
    START_UPDATES_AFTER = 0;

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
        }  else if (arg_name == "freq-updates") {
            FREQUENCY = atoi(arg_value.c_str());
        } else if (arg_name == "num-queries") {
            NUM_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "num-updates") {
            NUM_UPDATES = atoi(arg_value.c_str());
        } else if (arg_name == "column-size") {
            COLUMN_SIZE = atoi(arg_value.c_str());
        } else if (arg_name == "indexing-type") {
            INDEXING_TYPE = atoi(arg_value.c_str());
        } else if (arg_name == "delta") {
            DELTA = atof(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }


    int total;

    RangeQuery rangequeries;
    load_queries(&rangequeries,QUERIES_FILE_PATH,NUM_QUERIES);
    Column c;
    c.data = vector<int64_t>(COLUMN_SIZE);
    load_column(&c,COLUMN_FILE_PATH,COLUMN_SIZE);

    vector<int64_t> answers;
    load_answers(&answers,ANSWER_FILE_PATH,NUM_QUERIES);

    if (!RUN_CORRECTNESS)
        repetition = 5;

    vector<double> times(NUM_QUERIES);
    vector<double> deltas(NUM_QUERIES);

    switch (INDEXING_TYPE) {
        case PROGRESSIVE_UPDATE:
            progressive_update(c, rangequeries, answers, updates, times, range_query_incremental_quicksort);
            break;
        case CRACKING_MERGE_COMPLETE:
            cracking_update(c, rangequeries, answers, updates, times);
            break;
        case CRACKING_MERGE_GRADUALLY:
            cracking_update(c, rangequeries, answers, updates, times);
            break;
        case CRACKING_MERGE_RIPPLE:
            cracking_update(c, rangequeries, answers, updates, times);
            break;
    }

}