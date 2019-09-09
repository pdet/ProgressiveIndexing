#pragma once

#include "../structs.h"
#include "../util/avl_tree.h"

enum class CrackEngineType : uint8_t { Branched = 0, Predicated = 1, Rewired = 2 };

void exchange(IndexEntry *&c, int64_t x1, int64_t x2);

int64_t crackInTwoBranched(IndexEntry *&c, int64_t posL, int64_t posH, int64_t med);

int64_t crackInTwoPredicated(IndexEntry *&c, int64_t posL, int64_t posH, int64_t pivot);

CrackEngineType adaptive_cracking_engine(IndexEntry *c, AvlTree T, int64_t low_query, int64_t high_query,
                                         int64_t data_size);