#pragma once
#include "../progressive/incremental.h"
#include "../structs.h"

extern int INTERACTIVITY_IS_PERCENTAGE;
extern double DELTA, INTERACTIVITY_THRESHOLD;
extern int64_t NUM_QUERIES;
extern TotalTime query_times;
extern size_t current_query;
extern int FIXED_BUDGET;

typedef ResultStruct (*progressive_function)(Column &c, int64_t low, int64_t high, double delta);

typedef double (*estimate_function)(Column &c, int64_t low, int64_t high, double delta);

void progressive_indexing(Column &column, RangeQuery &rangeQueries, std::vector<int64_t> &answers,
                          std::vector<double> &deltas, progressive_function function);

void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, std::vector<int64_t> &answers,
                                     std::vector<double> &deltas, progressive_function function,
                                     estimate_function estimate);
