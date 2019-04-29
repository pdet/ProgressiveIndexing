#ifndef PROGRESSIVEINDEXING_CRACKING_UTIL_H
#define PROGRESSIVEINDEXING_CRACKING_UTIL_H

#include "../structs.h"
#include "../util/avl_tree.h"
//#include "../util/rewiring.h"
//const bool UseHugepages = true;
//constexpr std::size_t CHUNK_SIZE = 2lu * 1024lu * 1024lu; // 2 MiB

enum class CrackEngineType : uint8_t { Branched = 0, Predicated = 1, Rewired = 2};


void exchange(IndexEntry *&c, int64_t x1, int64_t x2);

int64_t crackInTwoBranched(IndexEntry *&c, int64_t posL, int64_t posH, int64_t med);

int64_t crackInTwoPredicated(IndexEntry *&c, int64_t posL, int64_t posH, int64_t pivot);

CrackEngineType adaptive_cracking_engine(IndexEntry *c,AvlTree T,int64_t low_query, int64_t high_query,int64_t data_size);

//int64_t * crackInTwoRewired(const int64_t pivot, int64_t *begin, int64_t *end,
//                            rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &data,
//                            rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &vm_buffer);
#endif //PROGRESSIVEINDEXING_CRACKING_UTIL_H
