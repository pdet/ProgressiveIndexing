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


#pragma clang diagnostic ignored "-Wformat"

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
int64_t  COLUMN_SIZE,NUM_QUERIES;
int ALGORITHM;

void full_scan(Column& column, RangeQuery& rangeQueries, vector<int64_t> &answers, vector<double>& time) {
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        int64_t sum=0;
        for (size_t j = 0; j < COLUMN_SIZE; j++)
            if (column.data[j] >= rangeQueries.leftpredicate[i] && column.data[j] <=rangeQueries.rightpredicate[i])
                sum += column.data[j];
        end = chrono::system_clock::now();
        time[i] += chrono::duration<double>(end - start).count();
//        fprintf(stderr, "Query %zu\n", i);
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", i,answers[i], sum );
    }
}

int64_t scanQuery(IndexEntry *c, int64_t from, int64_t to){
    int64_t  sum = 0;
    for(int64_t i = from;i<=to;i++) {
        sum += c[i].m_key;
    }

    return sum;
}
void *fullIndex(IndexEntry *c){
    hybrid_radixsort_insert(c, COLUMN_SIZE);
    void *I = build_bptree_bulk(c, COLUMN_SIZE);

    return I;
}

void full_index(Column& column, RangeQuery& rangeQueries, vector<int64_t> &answers, vector<double>& time){
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *data = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        data[i].m_key = column.data[i];
        data[i].m_rowId = i;
    }
    BulkBPTree* T = (BulkBPTree*) fullIndex(data);
    end = chrono::system_clock::now();
    time[0] += chrono::duration<double>(end - start).count();
    for(int i=0;i<NUM_QUERIES;i++){
        // query
        start = chrono::system_clock::now();
        int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[i]);
        int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[i]);
        int64_t sum = scanQuery(data, offset1, offset2);
        end = chrono::system_clock::now();
        time[i] += chrono::duration<double>(end - start).count();
        if (sum != answers[i])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i,answers[i], sum );

    }
    free(data);
    free(T);
}

void print_help(int argc, char** argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --answers-path\n");
    fprintf(stderr, "   --num-queries\n");
    fprintf(stderr, "   --column-size\n");
    fprintf(stderr, "   --algorithm\n");

}

pair<string,string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return { delimited.substr(0, pos), delimited.substr(pos+1) };
}

int main(int argc, char** argv) {
    COLUMN_FILE_PATH = "column";
    QUERIES_FILE_PATH = "query";
    ANSWER_FILE_PATH = "answer";

    NUM_QUERIES = 1000;
    COLUMN_SIZE = 10000000;
    ALGORITHM = 0;
    int repetition = 1;

    for(int i = 1; i < argc; i++) {
        auto arg = string(argv[i]);
        if (arg.substr(0,2) != "--") {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
        arg = arg.substr(2);
        auto p = split_once(arg, '=');
        auto& arg_name = p.first; auto& arg_value = p.second;
        if (arg_name == "column-path") {
            COLUMN_FILE_PATH = arg_value;
        } else if (arg_name == "query-path") {
            QUERIES_FILE_PATH = arg_value;
        }else if (arg_name == "answer-path") {
            ANSWER_FILE_PATH = arg_value;
        }  else if (arg_name == "num-queries") {
            NUM_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "column-size") {
            COLUMN_SIZE = atoi(arg_value.c_str());
        } else if (arg_name == "algorithm") {
            ALGORITHM = atoi(arg_value.c_str());
        }
        else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    chrono::time_point<chrono::system_clock> start, middle, end;
    chrono::duration<double> elapsed_seconds;
    start = chrono::system_clock::now();

    int total;

    RangeQuery rangequeries;
    load_queries(&rangequeries,QUERIES_FILE_PATH,NUM_QUERIES);
    Column c;
    c.data = vector<int64_t>(COLUMN_SIZE);
    load_column(&c,COLUMN_FILE_PATH,COLUMN_SIZE);

    vector<int64_t> answers;
    load_answers(&answers,ANSWER_FILE_PATH,NUM_QUERIES);

    vector<double> times(NUM_QUERIES);
    for (size_t i = 0; i < repetition; i ++){
        switch(ALGORITHM){
            case 0:
                full_scan(c, rangequeries, answers, times);
                break;
        }
    }
#ifndef DEBUG
    for (size_t i = 0; i < NUM_QUERIES; i ++){
            cout << times[i]/repetition << "\n";
        }
#endif

}
