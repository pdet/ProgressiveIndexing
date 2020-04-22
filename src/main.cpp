
#include <algorithm>
#include <experiment/experiments.hpp>
#include <random>
using namespace std;
int main(){
	int column_size = pow(10,7);
	size_t query_amount = 1000;
    double query_selectivity = 0.01; // 1 = 100%
    auto algorithms = make_unique<vector<IndexId>>();
	unsigned seed = 1;
    //! Generate Column
    auto column = make_unique<vector<int64_t>>(column_size);
    for (size_t i = 0; i < column_size; i++) {
        column->at(i) = i;
    }
    shuffle(column->begin(), column->end(),default_random_engine(seed));

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
    Experiments experiment(move(column), move(queries),IndexId::PROGRESSIVEQUICKSORT);
	//! Run queries
    experiment.run();
	//! Print all results
	experiment.print_results();

	experiment.algorithm = IndexId::PROGRESSIVEQUICKSORTCM;
	//! Run queries
    experiment.run();
	//! Print all results
	experiment.print_results();
}