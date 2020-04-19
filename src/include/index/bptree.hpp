#pragma once
#include "index.hpp"
#include <vector>

class BPTree {

  private:
    std::unique_ptr<std::vector<std::unique_ptr<IdxCol>>> m_root;

  public:
    size_t elementsPerNode = 16384;
    size_t COLUMN_SIZE;

  public:
    BPTree(IdxColEntry* data, int64_t size);
    size_t binary_search_lt(IdxColEntry* data, int64_t key);
    size_t binary_search_gte(IdxColEntry* data, int64_t key);
};
