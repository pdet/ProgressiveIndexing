#pragma once

#include "../progressive/progressive_quicksort.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class RangeQuery {
  public:
    RangeQuery(int64_t left_predicate, int64_t right_predicate) : left_predicate(left_predicate), right_predicate(right_predicate){};
    int64_t left_predicate;
    int64_t right_predicate;
};

enum class IndexId : uint8_t {
    //! Progressive Quicksort
	PROGRESSIVEQUICKSORT = 1,
	PROGRESSIVEQUICKSORTCM = 2
};


class Experiments {
  public:
    Experiments(std::unique_ptr<std::vector<int64_t>> column, std::unique_ptr<std::vector<RangeQuery>> queries,
                IndexId algorithm);
    int64_t progressive_indexing(size_t query_it);
    void progressive_indexing_initialize();
    //! Runs all Experiments
    void run();
    //! Runs one query
    int64_t run_query(size_t query_it, int64_t &result);
    void print_results();
    //! Used algorithm
    IndexId algorithm;
    //! Original Column
    std::unique_ptr<std::vector<int64_t>> column;
    //! Queries to be executed
    std::unique_ptr<std::vector<RangeQuery>> queries;

    //! Progressive Index
    std::unique_ptr<ProgressiveQuicksort> progressiveIndex = nullptr;
    //! If we are running tests, if yes a bunch of checks are added to verify the tree and column state.
    bool test = false;

    double interactivity_threshold = 0;

    double delta = 0.1;
    //! Profiling
    std::unique_ptr<std::vector<double>> time;

};
