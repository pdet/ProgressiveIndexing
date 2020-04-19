#include "experiment/experiments.hpp"
#include "input/file_manager.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <queue>
#include <random>
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;
using namespace chrono;
void print_help(int argc, char** argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --answers-path\n");
    fprintf(stderr, "   --algorithm\n");
}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char** argv) {
    // --column-path=/Users/holanda/Documents/Projects/ProgressiveUpdates/generated_data/1000000/column
    // --query-path=bla --answer-path=bla --algorithm=1
    string column_path;
    string query_path;
    string answer_path;
    int algorithmId;

    //! obtain a time-based seed:
    unsigned seed = system_clock::now().time_since_epoch().count();

    size_t column_size = 10000000;
    size_t query_amount = 1000;

    double query_selectivity = 0.01; // 1 = 100%

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
        if (arg_name == "column-path") {
            column_path = arg_value;
        } else if (arg_name == "query-path") {
            query_path = arg_value;
        } else if (arg_name == "answer-path") {
            answer_path = arg_value;
        } else if (arg_name == "algorithm") {
            algorithmId = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    auto algorithms = make_unique<vector<IndexId>>();
    algorithms->push_back(IndexId::PROGRESSIVEINDEXING);

    //! Generate Column
    auto column = make_unique<vector<int64_t>>(column_size);
    for (size_t i = 0; i < column_size; i++) {
        column->at(i) = i;
    }
    shuffle(column->begin(), column->end(), std::default_random_engine(1));

    //! Actual Queries
    auto queries = make_unique<vector<RangeQuery>>();
    size_t selectivity_size = column_size * query_selectivity;
    size_t upperbound = column_size - selectivity_size;
    for (size_t i = 0; i < query_amount; i++) {
        int64_t leftKey = rand() % upperbound;
        int64_t rightKey = leftKey + selectivity_size;
        queries->push_back(RangeQuery(leftKey, rightKey));
    }

    //! Generate actual experiment class
    Experiments experiment(move(column), move(queries), move(algorithms));
    experiment.interactivity_threshold = 1.2;
    experiment.run();

    experiment.print_results();
}
