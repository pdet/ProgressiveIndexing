#pragma once

#include "../structs.h"

void *build_binary_tree(IndexEntry *c, int n);

int64_t binary_search(IndexEntry *c, int64_t key, int64_t lower, int64_t upper, bool *foundKey);

int64_t binary_search_lt(IndexEntry *c, int64_t key, int64_t start, int64_t end);

int64_t binary_search_lte(IndexEntry *c, int64_t key, int64_t start, int64_t end);

int64_t binary_search_gte(IndexEntry *c, int64_t key, int64_t start, int64_t end);

int64_t binary_search_lte(int64_t *c, int64_t key, int64_t start, int64_t end);

int64_t binary_search_gte(int64_t *c, int64_t key, int64_t start, int64_t end);
