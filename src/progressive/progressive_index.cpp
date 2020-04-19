#include "progressive/progressive_index.hpp"
#include <chrono>
using namespace std;
using namespace chrono;

void ProgressiveIndex::consolidate(ssize_t& remaining_swaps) { fullIndex = make_unique<BPTree>(column->data, column->size); }

ResultStruct ProgressiveIndex::consolidate_scan(int64_t low, int64_t high) {
    int64_t offset1 = fullIndex->binary_search_gte(column->data, low);
    int64_t offset2 = fullIndex->binary_search_lt(column->data, high);
    int64_t sum = column->scanQuery(offset1, offset2);
    if (isTest) {
        int64_t result =  column->full_scan(low, high);
        assert(sum == result);
    }
    ResultStruct res;
    res.push_back(sum);
    return res;
}
