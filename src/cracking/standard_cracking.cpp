#include "../include/cracking/standard_cracking.h"
#include <chrono>

//extern size_t current_query;
//extern TotalTime query_times;


AvlTree standardCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey) {
    std::chrono::time_point<std::chrono::system_clock> start, end;
//    start = std::chrono::system_clock::now();

    IntPair p1, p2;

    p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
    p2 = FindNeighborsLT(highKey, T, dataSize - 1);
//    end = std::chrono::system_clock::now();
//    query_times.idx_time[current_query].index_lookup += std::chrono::duration<double>(end - start).count();
//    start = std::chrono::system_clock::now();

    IntPair pivot_pair = NULL;

    if (p1->first == p2->first && p1->second == p2->second) {
        pivot_pair = crackInThreeItemWise(c, p1->first, p1->second, lowKey, highKey);
    } else {
        // crack in two
        pivot_pair = (IntPair) malloc(sizeof(struct int_pair));
        pivot_pair->first = crackInTwoItemWise(c, p1->first, p1->second, lowKey);
        pivot_pair->second = crackInTwoItemWise(c, pivot_pair->first, p2->second, highKey);
    }
//    end = std::chrono::system_clock::now();
//    query_times.idx_time[current_query].sort += std::chrono::duration<double>(end - start).count();
//    start = std::chrono::system_clock::now();

    T = Insert(pivot_pair->first, lowKey, T);
    T = Insert(pivot_pair->second, highKey, T);

    free(p1);
    free(p2);
    if (pivot_pair) {
        free(pivot_pair);
        pivot_pair = NULL;
    }
//    end = std::chrono::system_clock::now();
//    query_times.idx_time[current_query].index_update += std::chrono::duration<double>(end - start).count();
    return T;
}
