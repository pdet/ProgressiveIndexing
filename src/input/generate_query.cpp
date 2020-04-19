#include "input/file_manager.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

//#define verify
//#define debug

void random(vector<int64_t>* leftQuery, vector<int64_t>* rightQuery, vector<int64_t>* queryAnswer, vector<int64_t>* orderedColumn,
            int64_t maxLeftQueryVal) {
    for (int i = 0; i < NUM_QUERIES; i++) {
        int64_t answer;
        int64_t q1 = rand() % maxLeftQueryVal;
        int64_t q2 = q1 + selectivity * column_upperbound;
        leftQuery->push_back(q1);
        rightQuery->push_back(q2);
    }
}

void generate_query(vector<int64_t>* orderedColumn) {
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    vector<int64_t> leftQuery;
    vector<int64_t> rightQuery;
    vector<int64_t> queryAnswer;
    int64_t maxLeftQueryVal = 0;
    start = chrono::system_clock::now();
    for (int i = orderedColumn->size() - 1; i >= 0; i--) {
        if (maxLeftQueryVal < orderedColumn->size() * SELECTIVITY_PERCENTAGE)
            maxLeftQueryVal++;
        else {
            maxLeftQueryVal = orderedColumn->at(i);
            break;
        }
    }
    if (QUERIES_PATTERN == 1)
        random(&leftQuery, &rightQuery, &queryAnswer, orderedColumn, maxLeftQueryVal);
    else if (QUERIES_PATTERN == 2)
        sequential(&leftQuery, &rightQuery, &queryAnswer, orderedColumn, maxLeftQueryVal);
    else if (QUERIES_PATTERN == 3)
        skewed(&leftQuery, &rightQuery, &queryAnswer, orderedColumn, maxLeftQueryVal);
    else if (QUERIES_PATTERN == 4)
        mixed(&leftQuery, &rightQuery, &queryAnswer, orderedColumn, maxLeftQueryVal);
    end = chrono::system_clock::now();
#ifdef VERIFY
    verifySelectivity(orderedColumn, &leftQuery, &rightQuery, SELECTIVITY_PERCENTAGE);
#endif

#ifdef DEBUG
    for (size_t i = 0; i < NUM_QUERIES; i++)
        cout << i + 1 << ";" << leftQuery[i] << ";" << rightQuery[i] << "\n";
#endif

    FILE* f = fopen(QUERIES_FILE_PATH.c_str(), "w+");
    fwrite(&leftQuery[0], sizeof(int64_t), NUM_QUERIES, f);
    fwrite(&rightQuery[0], sizeof(int64_t), NUM_QUERIES, f);
    fclose(f);
    FILE* f_2 = fopen(ANSWER_FILE_PATH.c_str(), "w+");
    fwrite(&queryAnswer[0], sizeof(int64_t), NUM_QUERIES, f_2);
    fclose(f_2);

    elapsed_seconds = end - start;
    cout << "Creating Query Attr: " << elapsed_seconds.count() << "s\n";
}

void print_help(int argc, char** argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --answer-path\n");
    fprintf(stderr, "   --selectiviy\n");
    fprintf(stderr, "   --zipf-alpha\n");
    fprintf(stderr, "   --num-queries\n");
    fprintf(stderr, "   --queries-pattern\n");
    fprintf(stderr, "   --point-query\n");
    fprintf(stderr, "   --column-upperbound\n");
}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char** argv) {

    for (int i = 1; i < argc; i++) {
        auto arg = string(argv[i]);
        if (arg.substr(0, 2) != "--") {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
        arg = arg.substr(2);
        auto p = split_once(arg, '=');
        auto& arg_name = p.first;
        auto& arg_value = p.second;
        if (arg_name == "query-path") {
            QUERIES_FILE_PATH = arg_value;
        } else if (arg_name == "selectivity") {
            SELECTIVITY_PERCENTAGE = atof(arg_value.c_str());
        } else if (arg_name == "num-queries") {
            NUM_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "column-upperbound") {
            UPPERBOUND = atoi(arg_value.c_str());
        } else if (arg_name == "queries-pattern") {
            QUERIES_PATTERN = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    if (!file_exists(QUERIES_FILE_PATH)) {
        Column c;
        c.data = vector<int64_t>(COLUMN_SIZE);
        load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);
        sort(c.data.begin(), c.data.end());
        generate_query(&c.data);
    } else {
        fprintf(stderr, "File already exists, delete it first if you want to "
                        "generate it again.\n");
    }
}
