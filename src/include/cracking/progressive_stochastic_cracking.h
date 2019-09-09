#pragma once

#include "../structs.h"
#include "cracking_engine.h"
#include "stdio.h"

#include <cstdlib>

AvlTree progressiveStochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey,
                                      QueryOutput *qo, CrackEngineType engineType);

