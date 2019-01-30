#ifndef PROGRESSIVEINDEXING_HYBRID_RADIX_INSERT_SORT_H
#define PROGRESSIVEINDEXING_HYBRID_RADIX_INSERT_SORT_H

#include "../structs.h"

void hybrid_radixsort_insert(IndexEntry *c, int n);
bool itqs(int64_t *val, size_t *ind, size_t n);

#endif
