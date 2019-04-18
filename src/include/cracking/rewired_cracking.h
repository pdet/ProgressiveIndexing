#ifndef PROGRESSIVEINDEXING_REWIRED_CRACKING_HPP
#define PROGRESSIVEINDEXING_REWIRED_CRACKING_HPP

#include <algorithm>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <emmintrin.h>
#include <immintrin.h>
#include <iostream>
#include <iterator>
#include <map>
#include "stdio.h"
#include <cstdlib>
#include "../util/avl_tree.h"
#include "../structs.h"

AvlTree rewiredCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey);
#endif //PROGRESSIVEINDEXING_REWIRED_CRACKING_HPP
