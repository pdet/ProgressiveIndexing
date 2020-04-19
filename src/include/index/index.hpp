#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>

class IdxColEntry {
  public:
    int64_t m_key;
    int64_t m_rowId;

    IdxColEntry() : m_key(-1), m_rowId(-1) {}

    IdxColEntry(int64_t key, int64_t rowId) : m_key(key), m_rowId(rowId) {}

    //! Query comparisons
    bool operator>(int64_t& other) const { return m_key > other; }
    bool operator>=(int64_t& other) const { return m_key >= other; }
    bool operator<(int64_t& other) const { return m_key < other; }
    bool operator<=(int64_t& other) const { return m_key <= other; }
    bool operator!=(int64_t& other) const { return m_key != other; }
    bool operator==(int64_t& other) const { return m_key == other; }

    bool operator>(const IdxColEntry& other) const { return m_key > other.m_key; }
    bool operator>=(const IdxColEntry& other) const { return m_key >= other.m_key; }
    bool operator<(const IdxColEntry& other) const { return m_key < other.m_key; }
    bool operator<=(const IdxColEntry& other) const { return m_key <= other.m_key; }
    bool operator!=(const IdxColEntry& other) const { return m_key != other.m_key; }
};

class IdxCol {
  public:
    IdxCol(size_t size);
    int64_t find(int64_t element);
    void check_column();
    int64_t full_scan(int64_t posL, int64_t posH);
    int64_t scanQuery(size_t posL, size_t posH);
    IdxCol() : size(0), capacity(0), data(nullptr){};
    ~IdxCol() { free(data); }
    size_t size;
    size_t capacity;
    IdxColEntry* data;
};
