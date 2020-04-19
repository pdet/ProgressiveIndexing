#include <cassert>
#include <progressive/progressive_index.hpp>
#include "index/index.hpp"

using namespace std;

IdxCol::IdxCol(size_t size) : size(size), capacity(size) {
    data = (IdxColEntry*)malloc(sizeof(IdxColEntry) * this->size);
}

int64_t IdxCol::find(int64_t element) {
    for (size_t i = 0; i < size; ++i) {
        if (data[i].m_key == element) {
            return i;
        }
    }
    return -1;
}

int64_t IdxCol::scanQuery( size_t posL, size_t posH) {
    int64_t sum = 0;
    for (size_t i = posL; i <= posH; ++i) {
        sum += data[i].m_key;
    }
    return sum;
}

void IdxCol::check_column() {
    int64_t sum = 0;
    for (size_t i = 0; i < size; ++i) {
        sum += data[i].m_key;
    }
    // 499999500000
    assert(sum == (size - 1) * size / 2);
}
int64_t IdxCol::full_scan(int64_t posL, int64_t posH){
    ResultStruct result;
    for (size_t i = 0; i < size; ++i) {
        int match = data[i].m_key >= posL && data[i].m_key < posH;
        result.maybe_push_back(data[i].m_key,match);
    }
    return result.sum;
}