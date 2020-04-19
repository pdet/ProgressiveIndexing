#pragma once
#include <cstdint>
#include <index/index.hpp>
#include <vector>

int64_t binary_search(IdxColEntry* c, int64_t key, int64_t lower, int64_t upper, bool* foundKey);
int64_t binary_search(std::vector<IdxColEntry>& c, int64_t key, int64_t lower, int64_t upper, bool* foundKey);

int64_t binary_search_lt(IdxColEntry* c, int64_t key, int64_t start, int64_t end);
int64_t binary_search_lte(IdxColEntry* c, int64_t key, int64_t start, int64_t end);
int64_t binary_search_gte(std::vector<IdxColEntry>& c, int64_t key, int64_t start, int64_t end);

// int64_t binary_search_lte(std::vector<IdxColEntry>& c, int64_t key, int64_t start, int64_t end);
int64_t binary_search_gte(IdxColEntry* c, int64_t key, int64_t start, int64_t end);
