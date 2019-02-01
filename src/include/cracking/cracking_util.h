#ifndef PROGRESSIVEINDEXING_CRACKING_UTIL_H
#define PROGRESSIVEINDEXING_CRACKING_UTIL_H

#include "../structs.h"
#include "../util/avl_tree.h"

void exchange(IndexEntry *&c, int64_t x1, int64_t x2);

int crackInTwoItemWise(IndexEntry *&c, int64_t posL, int64_t posH, int64_t med);

void *malloc_huge(size_t size);

IntPair crackInThreeItemWise(IndexEntry *c, int64_t posL, int64_t posH, int64_t low, int64_t high);

#endif //PROGRESSIVEINDEXING_CRACKING_UTIL_H
