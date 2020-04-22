#define CATCH_CONFIG_MAIN //! This tells Catch to provide a main() - only do
                          //! this in one cpp file

#include <catch.hpp>
#include <algorithm>
#include <experiment/experiments.hpp>
#include <random>

using namespace std;
using namespace chrono;

//! obtain a time-based seed:
unsigned seed = 1;

size_t column_size = 1000000;
size_t query_amount = 1000;
double query_selectivity = 0.01; // 1 = 100%

int64_t full_scan(vector<int64_t>& c, int64_t posL, int64_t posH) {
    int64_t sum = 0;
    for (size_t i = 0; i < c.size(); ++i) {
        if (c[i] >= posL && c[i] < posH) {
            sum += c[i];
        }
    }
    return sum;
}

TEST_CASE("Progressive Indexing", "[PI]") {
    cout << "Progressive Indexing"<< endl;

    //! Generate Column
    auto column = make_unique<vector<int64_t>>(column_size);
    for (size_t i = 0; i < column_size; i++) {
        column->at(i) = i;
    }
    shuffle(column->begin(), column->end(), std::default_random_engine(seed));

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
    Experiments experiment(move(column), move(queries), IndexId::PROGRESSIVEQUICKSORT);


    //! Set test flag on
    experiment.test = true;
    //! Let's check some answers
    for (size_t i = 0; i < experiment.queries->size(); i++) {
        auto left_predicate = experiment.queries->at(i).left_predicate;
        auto right_predicate = experiment.queries->at(i).right_predicate;
		int64_t result;
		experiment.run_query(i,result);
        int64_t answer = full_scan(*experiment.column, left_predicate, right_predicate);
        REQUIRE(result == answer);
    }
}

TEST_CASE("Progressive Indexing Cost Model", "[PI]") {
    cout << "Progressive Indexing Cost Model" << endl;

    //! Generate Column
    auto column = make_unique<vector<int64_t>>(column_size);
    for (size_t i = 0; i < column_size; i++) {
        column->at(i) = i;
    }
    shuffle(column->begin(), column->end(), std::default_random_engine(seed));

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
    Experiments experiment(move(column), move(queries), IndexId::PROGRESSIVEQUICKSORTCM);

    //! Set test flag on
    // experiment.test = true;

    //! Set interactivity threshold
    experiment.interactivity_threshold = 1.2;
    //! Let's check some answers
    for (size_t i = 0; i < experiment.queries->size(); i++) {
        auto left_predicate = experiment.queries->at(i).left_predicate;
        auto right_predicate = experiment.queries->at(i).right_predicate;
        int64_t result;
		experiment.run_query(i,result);
        int64_t answer = full_scan(*experiment.column,  left_predicate, right_predicate);
        REQUIRE(result == answer);
    }
}
