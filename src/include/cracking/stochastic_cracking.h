#pragma once

#include "../structs.h"
#include "../util/avl_tree.h"
#include "cracking_engine.h"
#include "stdio.h"

#include <cstdlib>

AvlTree stochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey,
                           QueryOutput *qo, CrackEngineType engineType);
IntPair crackInTwoMDD1RBranched(IndexEntry *&c, int64_t posL, int64_t posH, int64_t low, int64_t high,
                                IndexEntry *&view, int64_t &view_size);
IntPair crackInTwoMDD1RPredicated(IndexEntry *&c, int64_t posL, int64_t posH, int64_t low, int64_t high,
                                  IndexEntry *&view, int64_t &view_size);
