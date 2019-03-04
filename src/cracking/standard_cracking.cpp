#include "../include/cracking/standard_cracking.h"

AvlTree standardCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey) {

    IntPair p1, p2;

    p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
    p2 = FindNeighborsLT(highKey, T, dataSize - 1);
    IntPair pivot_pair = NULL;

    if (p1->first == p2->first && p1->second == p2->second) {
        pivot_pair = crackInThreeItemWise(c, p1->first, p1->second, lowKey, highKey);
    } else {
        // crack in two
        pivot_pair = (IntPair) malloc(sizeof(struct int_pair));
        pivot_pair->first = crackInTwoItemWise(c, p1->first, p1->second, lowKey);
        pivot_pair->second = crackInTwoItemWise(c, pivot_pair->first, p2->second, highKey);
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
