#ifndef PROGIDX_BULKLOADING_BP_TREE_H
#define PROGIDX_BULKLOADING_BP_TREE_H

#include <vector>
#include "../structs.h"

typedef int64_t colKey_t;
typedef int64_t rowId_t;

typedef enum _NodeType {
    InnerNodeType,
    LeafNodeType
} NodeType;

class InnerNode;
class LeafNode;

class BPNode {

protected:

    const NodeType m_nodeType;

    int64_t m_currentNumberOfEntries;
    colKey_t* m_keys;

    int64_t m_currentNumberOfNodes;
    BPNode** m_pointers;

    BPNode(NodeType nodeType)
            : m_nodeType(nodeType)
            , m_currentNumberOfEntries(0)
            , m_keys(NULL)
            , m_currentNumberOfNodes(0)
            , m_pointers(NULL)
            , m_fatherNode(NULL)
    {}

    ~BPNode() {
        if(m_keys) {
            delete[] m_keys;
            m_keys = NULL;
        }
        if(m_pointers) {

        }
    }

public:

    static int64_t m_maxNumberOfEntries;

    BPNode* m_fatherNode;

    bool isFull();
    void addKey(const colKey_t& key);

    const int64_t& getKey(const int64_t position);
    void removeKey(const int64_t position);

    void addPointer(BPNode* const node);
    BPNode* const getPointer(const int64_t position);
    void removePointer(const int64_t position);

    const int64_t numberOfKeys();

    BPNode* split(BPNode*& root);
    const LeafNode* lookup(const colKey_t& key);

    const NodeType& getNodeType() const {
        return m_nodeType;
    }
};

class InnerNode : public BPNode {

public:

    InnerNode() : BPNode(InnerNodeType) {}
};

class LeafNode : public BPNode {

private:

    LeafNode* m_previous;
    LeafNode* m_next;
    bool m_isOverflowNode;
    IndexEntry* m_currentOffset;

public:

    LeafNode(IndexEntry* currentOffset)
            : BPNode(LeafNodeType)
            , m_previous(NULL)
            , m_next(NULL)
            , m_isOverflowNode(false)
            , m_currentOffset(currentOffset)
    {}

    LeafNode(IndexEntry* currentOffset, bool overflowNode)
            : BPNode(LeafNodeType)
            , m_previous(NULL)
            , m_next(NULL)
            , m_isOverflowNode(overflowNode)
            , m_currentOffset(currentOffset)
    {}

    void addRowId(const rowId_t& rowId);
    void addKey(const colKey_t& key);
    rowId_t getRowId(const colKey_t& key) const;
    rowId_t getGTE(const colKey_t& key) const;
    rowId_t getLT(const colKey_t& key) const;
    rowId_t getLTE(const colKey_t& key) const;

    void setNext(LeafNode* next);
    void setPrevious(LeafNode* previous);
    LeafNode* getPrevious();
    LeafNode* getNext();
    void setAsOverflowNode() { m_isOverflowNode = true; }

};



class BulkBPTree {

private:

    BPNode* m_root;
    LeafNode* m_currentLeaf;

public:

    typedef std::pair<colKey_t, rowId_t> keyValuePair_t;

public:

    BulkBPTree(IndexEntry* data, int64_t size);
    BulkBPTree(int64_t* data, int64_t size);
    rowId_t lookup(const colKey_t& key);
    rowId_t gte(const colKey_t& key);
    rowId_t lt(const colKey_t& key);
    rowId_t lte(const colKey_t& key);

};

void *build_bptree_bulk(IndexEntry *c, int64_t n);
void *build_bptree_bulk_int(int64_t* c, int64_t n);

#endif



