#ifndef PROGRESSIVEINDEXING_STOCHASTIC_CRACKING_H
#define PROGRESSIVEINDEXING_STOCHASTIC_CRACKING_H

#include "stdio.h"
#include <cstdlib>
#include "../util/avl_tree.h"
#include "../structs.h"
#include "cracking_engine.h"

AvlTree
stochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey, QueryOutput *qo);

#endif //PROGRESSIVEINDEXING_STOCHASTIC_CRACKING_H
