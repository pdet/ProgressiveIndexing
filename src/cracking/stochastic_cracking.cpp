#include "../include/cracking/stochastic_cracking.h"

int64_t randomNumber(int64_t max) {
    return 1 + (int64_t) (max * (double) rand() / (RAND_MAX + 1.0));
}

IntPair crackInTwoMDD1R(IndexEntry *&c, int64_t posL, int64_t posH, int64_t low, int64_t high, IndexEntry *&view,
                        int64_t &view_size) {

    int64_t L = posL;
    int64_t R = posH;
    int64_t a = low;
    int64_t b = high;

    view = (IndexEntry *) malloc(
            (R - L + 1) * sizeof(IndexEntry));        // initialize view to the maximum possible size
    int64_t size = 0;

    int64_t x = c[L + randomNumber(R - L + 1) - 1].m_key;


    while (L <= R) {
        while (L <= R && c[L] < x) {
            if (c[L] >= a && c[L] < b)
                view[size++] = c[L];
            L = L + 1;
        }
        while (L <= R && c[R] >= x) {
            if (c[R] >= a && c[R] < b)
                view[size++] = c[R];
            R = R - 1;
        }
        if (L < R)
            exchange(c, L, R);
    }

    view_size = size;

    // add crack on X at position L
    IntPair p = (IntPair) malloc(sizeof(struct int_pair));
    p->first = x;
    p->second = L - 1;
    return p;
}

AvlTree
stochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey, QueryOutput *qo) {

    IntPair p1, p2;

    p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
    p2 = FindNeighborsLT(highKey, T, dataSize - 1);

    IntPair pivot_pair = NULL;

    if (p1->first == p2->first && p1->second == p2->second) {
        pivot_pair = crackInTwoMDD1R(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);
        lowKey = pivot_pair->first;
        highKey = pivot_pair->first;
        pivot_pair->first = pivot_pair->second;
    } else {

        pivot_pair = (IntPair) malloc(sizeof(struct int_pair));

        IntPair pivot_pair1 = crackInTwoMDD1R(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);
        qo->middlePart = &c[p1->second + 1];
        int size2 = p2->first - p1->second - 1;
        qo->middlePart_size = size2;
        IntPair pivot_pair2 = crackInTwoMDD1R(c, p2->first, p2->second, lowKey, highKey, qo->view2, qo->view_size2);

        pivot_pair->first = pivot_pair1->second;
        lowKey = pivot_pair1->first;
        pivot_pair->second = pivot_pair2->second;
        highKey = pivot_pair2->first;

        free(pivot_pair1);
        free(pivot_pair2);
    }

    T = Insert(pivot_pair->first, lowKey, T);
    T = Insert(pivot_pair->second, highKey, T);

    free(p1);
    free(p2);
    if (pivot_pair) {
        free(pivot_pair);
        pivot_pair = NULL;
    }

    return T;
}

