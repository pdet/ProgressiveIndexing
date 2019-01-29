#ifndef PROGRESSIVEINDEXING_STANDARD_CRACKING_H
#define PROGRESSIVEINDEXING_STANDARD_CRACKING_H
#include "stdio.h"
#include <cstdlib>
#include "../util/avl_tree.h"
#include "../structs.h"
#include "cracking_util.h"

AvlTree standardCracking(IndexEntry*& c, int dataSize, AvlTree T, int lowKey, int highKey);

#endif //PROGRESSIVEINDEXING_STANDARD_CRACKING_H
