#include "experiment/experiments.hpp"
#include "progressive/progressive_quicksort.hpp"
#include <iostream>
#include <string>
using namespace std;
using namespace chrono;
// Experiments::Experiments(string column_path, string query_path, int
// algorithm_id){
//  load_column(column_path);
//}
Experiments::Experiments(unique_ptr<vector<int64_t>> column, unique_ptr<vector<RangeQuery>> queries, IndexId algorithm)
    : column(move(column)), queries(move(queries)), algorithm(algorithm) {}

void Experiments::progressive_indexing_initialize() {

    if (interactivity_threshold == 0) {
        progressiveIndex = make_unique<ProgressiveQuicksort>(delta, test);
    } else {
        //! FIXME: hacky, should change the original-column representation
        unique_ptr<IdxCol> originalColumn = make_unique<IdxCol>(column->size());
        for (size_t i = 0; i < column->size(); i ++){
            originalColumn->data[i].m_rowId = i;
            originalColumn->data[i].m_key = (*column)[i];
        }
        auto startTimer = std::chrono::system_clock::now();
        int64_t sum = originalColumn->full_scan(10000,11000);
        auto endTimer = std::chrono::system_clock::now();
        //! Avoid -O3 opt
        if (sum != 0) {
            fprintf(stderr, " ");
        }
        auto full_scan_time = duration<double, std::milli>(endTimer - startTimer).count();
        progressiveIndex = make_unique<ProgressiveQuicksort>(interactivity_threshold, full_scan_time, test);
    }
}
int64_t Experiments::progressive_indexing(size_t query_it) {
    auto low = queries->at(query_it).left_predicate;
    auto high = queries->at(query_it).right_predicate;
    return  progressiveIndex->run_query(*column.get(), low, high,*time.get(),query_it);
}
void Experiments::run() {
    //! Initialize Profiling
    time = make_unique<std::vector<double>>(queries->size());

    //! Run Algorithm
	switch (algorithm) {
        case IndexId ::PROGRESSIVEQUICKSORT: {
		    interactivity_threshold =0;
            for (size_t i = 0; i < queries->size(); i++) {
                progressive_indexing(i);
            }
            break;
        }
	    case IndexId ::PROGRESSIVEQUICKSORTCM: {
		    interactivity_threshold = 1.2;
            progressive_indexing_initialize();
            for (size_t i = 0; i < queries->size(); i++) {
                progressive_indexing(i);
            }
            break;
        }
        default:
            throw invalid_argument("Algorithm Does Not Exist");
        }

}

int64_t Experiments::run_query(size_t query_it, int64_t &result) {
    //! Initialize Profiling if we are running first query
    if (query_it == 0) {
        progressive_indexing_initialize();
        time = make_unique<std::vector<double>>(queries->size());
    }
        switch (algorithm) {
        case IndexId ::PROGRESSIVEQUICKSORT: {
		    interactivity_threshold =0;
            return progressive_indexing(query_it);
        }
		case IndexId ::PROGRESSIVEQUICKSORTCM: {
		    interactivity_threshold = 1.2;
            return progressive_indexing(query_it);
        }
        default:
            throw invalid_argument("Algorithm Does Not Exist");
        }

}
void Experiments::print_results() {
	int alg_id;
	switch (algorithm) {
	case IndexId ::PROGRESSIVEQUICKSORT: {
		alg_id = 1;
		break;
	}
	case IndexId ::PROGRESSIVEQUICKSORTCM: {
		alg_id = 2;
		break;
	}
	default:
            throw invalid_argument("Algorithm Does Not Exist");
	}
        //! algorithm, query#, time
    for (size_t i = 0; i < time->size(); i++) {
        cout << alg_id<< "," << i << "," << time->at(i)<< "\n";
    }
};

//
// void Experiments::load_column(string column_path){
//  FILE *f = fopen(column_path.c_str(), "r");
//  if (!f) {
//    printf("Cannot open file.\n");
//    return;
//  }
//  int64_t *column_size = (int64_t *) malloc(sizeof(int64_t) * 1);
//  fread(column_size, sizeof(int64_t), 1, f);
//
//  int64_t *temp_data = (int64_t *) malloc(sizeof(int64_t) * column_size[0]);
//  fread(temp_data, sizeof(int64_t), column_size[0]+1, f);
//  //! initialize Column
//  column = make_unique<vector<int64_t>>(column_size[0]);
//  for (size_t i = 0; i < column_size[0]; i++) {
//    column->at(i) = temp_data[i+1];
//  }
//  fclose(f);
//}
////
////void Experiments::load_queries(string query_path){
////  FILE *f = fopen(QUERIES_FILE_PATH.c_str(), "r");
////  if (!f) {
////    printf("Cannot open file.\n");
////    return;
////  }
////  int64_t *temp_data = (int64_t *) malloc(sizeof(int64_t) * NUM_QUERIES);
////  fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
////  rangequeries->leftpredicate = vector<int64_t>(NUM_QUERIES);
////  for (size_t i = 0; i < NUM_QUERIES; i++) {
////    rangequeries->leftpredicate[i] = temp_data[i];
////  }
////  fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
////  rangequeries->rightpredicate = vector<int64_t>(NUM_QUERIES);
////  for (size_t i = 0; i < NUM_QUERIES; i++) {
////    rangequeries->rightpredicate[i] = temp_data[i];
////  }
////  fclose(f);
////
////}
////
////void Experiments::load_answers(string answer_path){
////  FILE *f = fopen(QUERIES_ANSWERS_PATH.c_str(), "r");
////  if (!f) {
////    printf("Cannot open file.\n");
////    return;
////  }
////  int64_t *temp_data = (int64_t *) malloc(sizeof(int64_t) * NUM_QUERIES);
////  fread(temp_data, sizeof(int64_t), NUM_QUERIES, f);
////  for (size_t i = 0; i < NUM_QUERIES; i++) {
////    answers->push_back(temp_data[i]);
////  }
////  fclose(f);
////}
//
