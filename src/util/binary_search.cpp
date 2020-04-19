#include "util/binary_search.hpp"

int64_t binary_search(IdxColEntry* c, int64_t key, int64_t lower, int64_t upper, bool* foundKey) {
    *foundKey = false;
    upper--;
    while (lower <= upper) {
        int middle = (lower + upper) / 2;
        auto middleElement = c[middle];

        if (middleElement < key) {
            lower = middle + 1;
        } else if (middleElement > key) {
            upper = middle - 1;
        } else {
            *foundKey = true;
            return middle;
        }
    }
    return upper;
}

int64_t binary_search(std::vector<IdxColEntry>& c, int64_t key, int64_t lower, int64_t upper, bool* foundKey) {
    *foundKey = false;
    while (lower <= upper) {
        int middle = (lower + upper) / 2;
        auto middleElement = c[middle];

        if (middleElement < key) {
            lower = middle + 1;
        } else if (middleElement > key) {
            upper = middle - 1;
        } else {
            *foundKey = true;
            return middle;
        }
    }
    return upper;
}

int64_t binary_search_lt(IdxColEntry* c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);
    if (found) {
        while (pos >= start && c[pos] == key){
            pos --;
        }
    }
    return pos;
}

int64_t binary_search_lte(IdxColEntry* c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);

    while (c[pos] <= key)
        pos++;
    pos--;
    return pos;
}

int64_t binary_search_gte(IdxColEntry* c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);
    if (found) {
        while (--pos >= start && c[pos] == key)
            ;
    }
    ++pos;
    return pos;
}

int64_t binary_search_gte(std::vector<IdxColEntry>& c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);
    if (found) {
        while (--pos >= start && c[pos] == key)
            ;
    }
    ++pos;
    return pos;
}

int64_t binary_search(int64_t* c, int64_t key, int64_t lower, int64_t upper, bool* foundKey) {

    *foundKey = false;
    upper--;
    while (lower <= upper) {
        int middle = (lower + upper) / 2;
        auto middleElement = c[middle];

        if (middleElement < key) {
            lower = middle + 1;
        } else if (middleElement > key) {
            upper = middle - 1;
        } else {
            *foundKey = true;
            return middle;
        }
    }
    return upper;
}

int64_t binary_search_lte(int64_t* c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);
    while (pos < end && c[pos] <= key)
        pos++;
    pos--;

    return pos;
}

int64_t binary_search_gte(int64_t* c, int64_t key, int64_t start, int64_t end) {
    bool found = false;
    int pos = binary_search(c, key, start, end, &found);
    if (found) {
        while (--pos >= start && c[pos] == key)
            ;
    }
    ++pos;
    return pos;
}
