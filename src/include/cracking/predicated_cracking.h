//
// Created by Pedro Holanda on 17/04/19.
//

#ifndef PROGRESSIVEINDEXING_PREDICATED_CRACKING_HPP
#define PROGRESSIVEINDEXING_PREDICATED_CRACKING_HPP

#include "stdio.h"
#include <cstdlib>
#include "../util/avl_tree.h"
#include "../structs.h"

AvlTree predicatedCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey);

#endif //PROGRESSIVEINDEXING_PREDICATED_CRACKING_HPP
