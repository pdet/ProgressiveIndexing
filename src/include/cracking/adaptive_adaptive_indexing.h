//
// Created by Pedro Holanda on 03/05/19.
//

#ifndef PROGRESSIVEINDEXING_ADAPTIVE_ADAPTIVE_INDEXING_H
#define PROGRESSIVEINDEXING_ADAPTIVE_ADAPTIVE_INDEXING_H
#include "../structs.h"
#include "index.h"


// L1 CACHE INFO
static const size_t L1_CACHE_SIZE = 32;
static const size_t L1_LINE_SIZE = 32;

// L2 CACHE INFO
static const size_t L2_CACHE_SIZE = 256;
// PAGE INFO
static const offset_t SMALL_PAGE_SIZE = 4096;
static const offset_t PAGE_SIZE = 2097152;
static const offset_t KEY_SIZE = 2 * sizeof(uint64_t);
static const offset_t EPP = PAGE_SIZE / KEY_SIZE; // elements per page
static const offset_t ELEMENTS_IN_L1 = (EPP * L1_CACHE_SIZE) / PAGE_SIZE;
static const offset_t ELEMENTS_IN_L2 = (EPP * L2_CACHE_SIZE) / PAGE_SIZE;

// DTLB1 INFO (4K=small pages, 2MB=large pages) => (64 entries, 32 entries)
static const offset_t NUM_DTLB1_ENTRIES = 32;//(PAGE_SIZE > SMALL_PAGE_SIZE ? 32 : 64);
static const offset_t ELEMENTS_IN_TLB = (NUM_DTLB1_ENTRIES * PAGE_SIZE) / KEY_SIZE;

void copy_key_column(row_t *src, crow_t *dst, offset_t size, entry_t &max);
void init_workingData(wdPartitioned_t *const workingData, offset_t size);
offset_t findInitialShiftOffset(entry_t key);
entry_t FILTER_helper(crow_t *dst, offset_t size, entry_t keyL, entry_t keyH, crackerIndex_pt index);
entry_t DECISION_helper_simple(crow_t *dst, offset_t size, entry_t keyL, entry_t keyH, crackerIndex_pt index, ADAPT_FUNC F
        , wdPartitioned_t *const workingData);
void EQUIDEPTHOOPRADIX_helper(crow_t *in, crow_t *out, offset_t size,
                              offset_t shiftOffset, crackerIndex_pt index, wdPartitioned_t *const workingData);
offset_t FLINEAR(const working_area_t p, const offset_t numBits, const offset_t bMin, const offset_t bMax, const offset_t sThreshold);
#endif //PROGRESSIVEINDEXING_ADAPTIVE_ADAPTIVE_INDEXING_H
