//
// Created by Pedro Holanda on 31/01/19.
//

#ifndef PROGRESSIVEINDEXING_CRACKING_UPDATES_H
#define PROGRESSIVEINDEXING_CRACKING_UPDATES_H
#include "stdio.h"
#include <cstdlib>
#include "../util/avl_tree.h"
#include "../structs.h"
#include "cracking_util.h"
void merge(IndexEntry *&column, size_t &capacity, AvlTree T, Column &updates, int64_t posL, int64_t posH, int64_t _next = -1);
void merge_ripple(IndexEntry *&column,size_t &capacity, AvlTree T, Column &updates, int64_t posL, int64_t posH, int64_t low, int64_t high);
#endif //PROGRESSIVEINDEXING_CRACKING_UPDATES_H
