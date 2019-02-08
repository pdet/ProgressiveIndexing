#ifndef PROGRESSIVEINDEXING_STRUCTS_H_
#define PROGRESSIVEINDEXING_STRUCTS_H_

#pragma once

#include <vector>
#include <cstdlib>
#include <iostream>

#include <iomanip>

#include <cmath>
#include <queue>
#include <climits>
#include <algorithm>

struct QuicksortNode {
    int64_t pivot;
    int64_t min = INT_MIN;
    int64_t max = INT_MAX;
    size_t start;
    size_t end;
    size_t current_start, current_end;
    bool sorted;

    int64_t position;
    int64_t parent;
    int64_t left;
    int64_t right;

    QuicksortNode() : position(-1), parent(-1), left(-1), right(-1), sorted(false), min(INT_MIN), max(INT_MAX) {}

    QuicksortNode(int64_t position) : position(position), parent(-1), left(-1), right(-1), sorted(false), min(INT_MIN),
                                      max(INT_MAX) {}

    QuicksortNode(int64_t position, int64_t parent) : position(position), parent(parent), left(-1), right(-1),
                                                      sorted(false), min(INT_MIN), max(INT_MAX) {}
};

struct IncrementalQuicksortIndex {
    std::vector<QuicksortNode> nodes;
    QuicksortNode root;
    size_t *index = nullptr;
    int64_t *data = nullptr;
    size_t current_position = 0;
    size_t current_pivot = 0;

    IncrementalQuicksortIndex() : index(nullptr), data(nullptr), current_pivot(0) {}

    void clear();
};

struct Column {
    std::vector<int64_t> data;
    IncrementalQuicksortIndex qs_index;
    bool converged = false;
    int64_t *final_data = nullptr;

    void Clear() {
//        if(updates)
//            data.clear();
        converged = false;
        final_data = nullptr;
        qs_index.clear();
    };


};

struct RangeQuery {
    std::vector<int64_t> leftpredicate;
    std::vector<int64_t> rightpredicate;
};

class IndexEntry {
public:


    int64_t m_key;
    int64_t m_rowId;

    IndexEntry(int64_t i)
            : m_key(i), m_rowId(-1) {
    }

    IndexEntry()
            : m_key(-1), m_rowId(-1) {}

    IndexEntry(int64_t key, int64_t rowId)
            : m_key(key), m_rowId(rowId) {}

//Query comparisons
    bool operator>(int64_t &other) const { return m_key > other; }

    bool operator>=(int64_t &other) const { return m_key >= other; }

    bool operator<(int64_t &other) const { return m_key < other; }

    bool operator<=(int64_t &other) const { return m_key <= other; }

    bool operator!=(int64_t &other) const { return m_key != other; }

    bool operator==(int64_t &other) const { return m_key == other; }

    bool operator>(const IndexEntry &other) const { return m_key > other.m_key; }

    bool operator>=(const IndexEntry &other) const { return m_key >= other.m_key; }

    bool operator<(const IndexEntry &other) const { return m_key < other.m_key; }

    bool operator<=(const IndexEntry &other) const { return m_key <= other.m_key; }

    bool operator!=(const IndexEntry &other) const { return m_key != other.m_key; }

};

struct int_pair {
    int64_t first;
    int64_t second;
};
typedef struct int_pair *IntPair;

typedef int64_t ElementType;

struct AvlNode;

typedef struct AvlNode *PositionAVL;
typedef struct AvlNode *AvlTree;

struct AvlNode {
    ElementType Element;
    int64_t offset;

    AvlTree Left;
    AvlTree Right;
    int64_t Height;
};

// Used for stochastic cracking to check its views
struct QueryOutput {
    int64_t sum;                    // stores the sum result
    IndexEntry *view1;                    // stores a materialized view of the lower part
    int64_t view_size1;            // stores the size of the view of the lower part
    IndexEntry *middlePart;                // if there is a middle part, store the address
    int64_t middlePart_size;        // and the corresponding size here
    IndexEntry *view2;                    // stores a materialized view of the upper part
    int64_t view_size2;            // stores the size of the view of the upper part
};

struct ResultStruct {
    int64_t sum = 0;

    void reserve(size_t capacity) {
        (void) capacity;
    }

    size_t size() { return 1; }

    int64_t *begin() { return &sum; }

    int64_t *end() { return &sum + 1; }

    inline void push_back(int64_t value) {
        sum += value;
    }

    inline void maybe_push_back(int64_t value, int maybe) {
        sum += maybe * value;
    }

    inline void merge(ResultStruct other) {
        sum += other.sum;
    }

    int64_t &operator[](const size_t index) {
        return sum;
    }

    const int64_t operator[](const size_t index) const {
        return sum;
    }


    ResultStruct() : sum(0) {}
};


#endif