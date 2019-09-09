#include "../include/util/file_manager.h"

#include "../include/structs.h"

#include <vector>

using namespace std;

void load_queries(RangeQuery *rangequeries, string QUERIES_FILE_PATH, int64_t NUM_QUERIES) {
	FILE *f = fopen(QUERIES_FILE_PATH.c_str(), "r");
	if (!f) {
		printf("Cannot open file.\n");
		return;
	}
	int64_t *temp_data = (int64_t *)malloc(sizeof(int64_t) * NUM_QUERIES);
	fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
	rangequeries->leftpredicate = vector<int64_t>(NUM_QUERIES);
	for (size_t i = 0; i < NUM_QUERIES; i++) {
		rangequeries->leftpredicate[i] = temp_data[i];
	}
	fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
	rangequeries->rightpredicate = vector<int64_t>(NUM_QUERIES);
	for (size_t i = 0; i < NUM_QUERIES; i++) {
		rangequeries->rightpredicate[i] = temp_data[i];
	}
	fclose(f);
}

void load_column(Column *c, string COLUMN_FILE_PATH, int64_t COLUMN_SIZE) {
	FILE *f = fopen(COLUMN_FILE_PATH.c_str(), "r");
	if (!f) {
		printf("Cannot open file.\n");
		return;
	}
	int *temp_data = (int *)malloc(sizeof(int) * COLUMN_SIZE);
	fread(temp_data, sizeof(int), COLUMN_SIZE, f);
	c->data = vector<int64_t>(COLUMN_SIZE);
	c->min = std::numeric_limits<int64_t>::max();
	c->max = std::numeric_limits<int64_t>::min();
	for (size_t i = 0; i < COLUMN_SIZE; i++) {
		c->data[i] = temp_data[i];
		if (c->data[i] < c->min) {
			c->min = c->data[i];
		}
		if (c->data[i] > c->max) {
			c->max = c->data[i];
		}
	}
	fclose(f);
}

void load_answers(vector<int64_t> *answers, string QUERIES_ANSWERS_PATH, int64_t NUM_QUERIES) {
	FILE *f = fopen(QUERIES_ANSWERS_PATH.c_str(), "r");
	if (!f) {
		printf("Cannot open file.\n");
		return;
	}
	int64_t *temp_data = (int64_t *)malloc(sizeof(int64_t) * NUM_QUERIES);
	fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
	for (size_t i = 0; i < NUM_QUERIES; i++) {
		answers->push_back(temp_data[i]);
	}
	fclose(f);
}

bool file_exists(const string &name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}
}
