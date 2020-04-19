#include "index/bptree.hpp"

#include "util/binary_search.hpp"

#include <cassert>
#include <cstring>

using namespace std;

size_t BPTree::binary_search_gte(IdxColEntry* data, int64_t key) {
    int rootLevel = m_root->size() - 1;
    size_t offset_low = 0;
    size_t offset_high = (*m_root)[rootLevel]->size;
    for (; rootLevel >= 0; rootLevel--) {
        size_t localOffset = ::binary_search_lt((*m_root)[rootLevel]->data, key, offset_low, offset_high);
        offset_low = (*m_root)[rootLevel]->data[localOffset].m_rowId;
        if (localOffset == (*m_root)[rootLevel]->size - 1) {
            if (rootLevel == 0) {
                offset_high = COLUMN_SIZE;
            } else {
                offset_high = (*m_root)[rootLevel - 1]->size;
            }
        } else {
            assert(localOffset + 1 < (*m_root)[rootLevel]->size);
            offset_high = (*m_root)[rootLevel]->data[localOffset + 1].m_rowId;
        }
    }
    //! One last BS to find the actual position in the vector
    return ::binary_search_gte(data, key, offset_low, offset_high);
};

size_t BPTree::binary_search_lt(IdxColEntry* data, int64_t key) {
    int rootLevel = m_root->size() - 1;
    size_t offset_low = 0;
    size_t offset_high = (*m_root)[rootLevel]->size;
    for (; rootLevel >= 0; rootLevel--) {
        size_t localOffset = ::binary_search_lt((*m_root)[rootLevel]->data, key, offset_low, offset_high);
        offset_low = (*m_root)[rootLevel]->data[localOffset].m_rowId;
        if (localOffset == (*m_root)[rootLevel]->size - 1) {
            if (rootLevel == 0) {
                offset_high = COLUMN_SIZE;
            } else {
                offset_high = (*m_root)[rootLevel - 1]->size;
            }
        } else {
            assert(localOffset + 1 < (*m_root)[rootLevel]->size);
            offset_high = (*m_root)[rootLevel]->data[localOffset + 1].m_rowId;
        }
    }
    //! One last BS to find the actual position in the vector
    return ::binary_search_lt(data, key, offset_low, offset_high);
};

BPTree::BPTree(IdxColEntry* data, int64_t size) : COLUMN_SIZE(size) {
    m_root = make_unique<std::vector<std::unique_ptr<IdxCol>>>();
    size_t level_size = size;
    do {
        level_size = ceil((double)level_size / elementsPerNode);
        auto level_data = std::make_unique<IdxCol>(level_size);
        size_t cur_element = 0;
        for (size_t i = 0; i < size; i += elementsPerNode) {
            level_data->data[cur_element].m_key = data[i].m_key;
            level_data->data[cur_element++].m_rowId = i;
        }
        m_root->push_back(move(level_data));

    } while (level_size > elementsPerNode);
}
