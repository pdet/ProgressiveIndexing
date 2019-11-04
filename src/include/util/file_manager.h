#pragma once

#include "../structs.h"

#include <iostream>

using namespace std;

void load_queries(RangeQuery *rangequeries, string QUERIES_FILE_PATH, int64_t NUM_QUERIES);

void load_column(Column *c, string COLUMN_FILE_PATH, int64_t COLUMN_SIZE);

void load_answers(vector<int64_t> *answers, string QUERIES_ANSWERS_PATH, int64_t NUM_QUERIES);

bool file_exists(const string &name);
