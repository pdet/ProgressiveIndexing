#ifndef PROGRESSIVEINDEXING_PROGRESSIVE_STOCHASTIC_CRACKING_H
#define PROGRESSIVEINDEXING_PROGRESSIVE_STOCHASTIC_CRACKING_H

#include "stdio.h"
#include <cstdlib>
#include "../structs.h"
#include "cracking_util.h"


AvlTree progressiveStochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey,
                                      QueryOutput *qo);

#endif //PROGRESSIVEINDEXING_PROGRESSIVE_STOCHASTIC_CRACKING_H
