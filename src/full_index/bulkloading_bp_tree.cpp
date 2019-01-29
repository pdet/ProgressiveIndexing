#include "../include/full_index/bulkloading_bp_tree.h"
#include "../include/util/binary_search.h"
#include <cstring>

int64_t BPTREE_ELEMENTSPERNODE = 16384;
extern int64_t COLUMN_SIZE;
int64_t binary_search_pure(colKey_t *c, int64_t key, int64_t lower, int64_t upper, bool* foundKey) {

    *foundKey = false;
    // binary search: iterative version with early termination
    while(lower <= upper) {
        int64_t middle = (lower + upper) / 2;
        colKey_t middleElement = c[middle];

        if(middleElement < key) {
            lower = middle + 1;
        }
        else if(middleElement > key) {
            upper = middle - 1;
        }
        else {
            *foundKey = true;
            return middle;
        }
    }
    return upper;
}

int64_t binary_search_gt_pure(colKey_t *c, int64_t key, int64_t start, int64_t end){
    bool found = false;
    int64_t pos = binary_search_pure(c, key, start, end, &found);
    if(found) {
        // go to the first element after the found ones
        while(++pos <= end && c[pos] == key);
    }
    else {
        // To get to the first c[pos] == key element.
        ++pos;
    }

    return pos;
}

bool BPNode::isFull() {
    return m_currentNumberOfEntries == BPNode::m_maxNumberOfEntries;
}

void BPNode::addKey(const colKey_t& key) {
    if(!m_keys) {
        m_keys = new colKey_t[BPNode::m_maxNumberOfEntries];
    }
    if(m_currentNumberOfEntries < BPNode::m_maxNumberOfEntries) {
        m_keys[m_currentNumberOfEntries++] = key;
    }
}

const colKey_t& BPNode::getKey(const int64_t position) {
    return m_keys[position];
}


void BPNode::removeKey(const int64_t position) {
    if(m_currentNumberOfEntries > 0) {
        memmove(m_keys + position, m_keys + position + 1, sizeof(colKey_t) * (m_currentNumberOfEntries - position - 1));
        --m_currentNumberOfEntries;
    }
}

void BPNode::addPointer(BPNode* const node) {
    if(!m_pointers) {
        m_pointers = new BPNode*[BPNode::m_maxNumberOfEntries];
    }
    m_pointers[m_currentNumberOfNodes++] = node;
}

BPNode* const BPNode::getPointer(const int64_t position) {
    return m_pointers[position];
}

void BPNode::removePointer(const int64_t position) {
    if(m_currentNumberOfNodes > 0) {
        memmove(m_pointers + position, m_pointers + position + 1, sizeof(BPNode*) * (m_currentNumberOfNodes - position - 1));
        --m_currentNumberOfNodes;
    }
}


void LeafNode::addKey(const colKey_t& key) {
    if(m_currentNumberOfEntries < BPNode::m_maxNumberOfEntries) {
        m_currentOffset[m_currentNumberOfEntries++].m_key = key;
    }
}

void LeafNode::addRowId(const rowId_t& rowId) {
    m_currentOffset[m_currentNumberOfEntries-1].m_rowId = rowId;
}

rowId_t LeafNode::getRowId(const colKey_t& key) const {
    bool foundKey = false;
    int64_t position = binary_search(m_currentOffset, key, 0, m_currentNumberOfEntries - 1, &foundKey);
    if(foundKey) {
        return m_currentOffset[position].m_rowId;
    }

    // the key was not found
    return -1;
}

rowId_t LeafNode::getGTE(const colKey_t& key) const {
    int64_t position = binary_search_gte(m_currentOffset, key, 0, m_currentNumberOfEntries - 1);
    if(position < m_currentNumberOfEntries) {
        return m_currentOffset[position].m_rowId;
    }
    else {
        // we have not found the entry in the current leaf, so try the next one
        if(m_next) {
            return m_next->getGTE(key);
        }
        else {
            // the key was not found
            return -1;
        }
    }
}

rowId_t LeafNode::getLTE(const colKey_t& key) const {
    int64_t position = binary_search_lte(m_currentOffset, key, 0, m_currentNumberOfEntries - 1);

    if(position < m_currentNumberOfEntries) {
        return m_currentOffset[position].m_rowId;
    }
    else {
        // we have not found the entry in the current leaf, so try the next one
        if(m_next) {
            return m_next->getLTE(key);
        }
        else {
            // the key was not found
            return -1;
        }
    }
}

rowId_t LeafNode::getLT(const colKey_t& key) const {
    int64_t position = binary_search_gte(m_currentOffset, key, 0, m_currentNumberOfEntries - 1);
    if(position < m_currentNumberOfEntries) {
        if(position > 0) {
            return m_currentOffset[position-1].m_rowId;
        }
        else {
            // we might be to the right of an overflow block
            if(m_previous) {
                return m_previous->m_currentOffset[m_previous->m_currentNumberOfEntries-1].m_rowId;
            }
            else {
                return -1;
            }
        }
    }
    else {
        // we have not found the entry in the current leaf, so try the next one
        if(m_next) {
            return m_next->getLT(key);
        }
        else {
            // the key was not found and we are the end, so return the data size
            return COLUMN_SIZE;
        }
    }
}

const int64_t BPNode::numberOfKeys() {
    return m_currentNumberOfEntries;
}

BPNode* BPNode::split(BPNode*& root) {
    BPNode* currentNode = this;
    BPNode* newNode = new InnerNode();
    BPNode* fatherNode = currentNode->m_fatherNode;
    if(!fatherNode) {
        // the currentNode is the root, so we have to create a new root
        fatherNode = new InnerNode();
        fatherNode->addPointer(currentNode);
        currentNode->m_fatherNode = fatherNode;
        root = fatherNode;
    }

    int64_t leftSize = BPNode::m_maxNumberOfEntries / 2;
    if(BPNode::m_maxNumberOfEntries % 2 != 0) {
        ++leftSize;
    }

    if(fatherNode->isFull()) {
        fatherNode = fatherNode->split(root);
    }

    newNode->m_fatherNode = fatherNode;

    // get new split key
    colKey_t splitKey = currentNode->getKey(leftSize);
    currentNode->removeKey(leftSize);

    // insert it into father
    fatherNode->addKey(splitKey);
    fatherNode->addPointer(newNode);

    // move the keys and the pointers that belong to the right side onto the right side
    while((int64_t) m_currentNumberOfNodes > leftSize + 1) {
        newNode->addPointer(currentNode->getPointer(leftSize + 1));
        currentNode->removePointer(leftSize + 1);
    }

    while(currentNode->m_currentNumberOfEntries > leftSize) {
        newNode->addKey(currentNode->getKey(leftSize));
        currentNode->removeKey(leftSize);
    }

    return newNode;
}

const LeafNode* BPNode::lookup(const colKey_t& key) {
    if(m_nodeType == LeafNodeType) {
        return static_cast<LeafNode*>(this);
    }
    else {
        int64_t position = binary_search_gt_pure(m_keys, key, 0, m_currentNumberOfEntries - 1);
        return m_pointers[position]->lookup(key);
    }
}

void LeafNode::setPrevious(LeafNode* previous) {
    m_previous = previous;
}

void LeafNode::setNext(LeafNode* next) {
    m_next = next;
}

LeafNode* LeafNode::getPrevious() {
    return m_previous;
}

LeafNode* LeafNode::getNext() {
    return m_next;
}

BulkBPTree::BulkBPTree(IndexEntry* data, int64_t size) {
    // assumes that the entries are already sorted by key

    BPNode::m_maxNumberOfEntries = BPTREE_ELEMENTSPERNODE;
    IndexEntry* entries = new IndexEntry[size];

    m_root = new InnerNode();
    m_root->m_fatherNode = NULL;

    m_currentLeaf = new LeafNode(entries);
    m_currentLeaf->m_fatherNode = m_root;

    m_root->addPointer(m_currentLeaf);

    LeafNode* lastOverflowNode = NULL;
    bool overflow = false;
    for(int64_t i = 0; i < size; ) {
        if(m_currentLeaf->isFull()) {
            // current leaf is full, so we have to create a new one
            LeafNode* newLeaf = new LeafNode(entries + i);

            if(m_currentLeaf->m_fatherNode->isFull()) {
                // there is no space left, so we have to split the father node recursively to be able to link the new leaf
                m_currentLeaf->m_fatherNode = m_currentLeaf->m_fatherNode->split(m_root);
            }

            // there is space left, so we can simply add the new leaf there, but only if it is not an overflow leaf
            if(data[i].m_key != (i > 0 ? data[i-1].m_key : -1)) {
                m_currentLeaf->m_fatherNode->addKey(data[i].m_key);
                m_currentLeaf->m_fatherNode->addPointer(newLeaf);

                newLeaf->m_fatherNode = m_currentLeaf->m_fatherNode;
                if(overflow) {
                    newLeaf->setPrevious(lastOverflowNode);
                    lastOverflowNode->setNext(newLeaf);
                    overflow = false;
                }
                else {
                    newLeaf->setPrevious(m_currentLeaf);
                    m_currentLeaf->setNext(newLeaf);
                }

                m_currentLeaf = newLeaf;
            }
            else {
                // we have an overflow due to duplicate keys, so fill up overflow leaves until
                // a different key arrives or all keys have been processed
                overflow = true;
                newLeaf->setAsOverflowNode();
                lastOverflowNode = newLeaf;
                newLeaf->m_fatherNode = NULL;
                newLeaf->setPrevious(m_currentLeaf);
                m_currentLeaf->setNext(newLeaf);
                while(i < size && data[i].m_key == (i > 0 ? data[i-1].m_key : -1)) {
                    if(newLeaf->isFull()) {
                        LeafNode* oldOverflow = newLeaf;
                        newLeaf = new LeafNode(entries + i, true);
                        lastOverflowNode = newLeaf;
                        newLeaf->m_fatherNode = NULL;
                        newLeaf->setPrevious(oldOverflow);
                        oldOverflow->setNext(newLeaf);
                    }
                    newLeaf->addKey(data[i].m_key);
                    newLeaf->addRowId(i);
                    ++i;
                }
                // the overflow nodes have been filled up, so get back to normal
            }
        }

        // space left in leaf, so simply insert
        if(!overflow) {
            m_currentLeaf->addKey(data[i].m_key);
            m_currentLeaf->addRowId(i);
            ++i;
        }
    }
}

BulkBPTree::BulkBPTree(int64_t* data, int64_t size) {
    // assumes that the entries are already sorted by key

    BPNode::m_maxNumberOfEntries = BPTREE_ELEMENTSPERNODE;
    IndexEntry* entries = new IndexEntry[size];

    m_root = new InnerNode();
    m_root->m_fatherNode = NULL;

    m_currentLeaf = new LeafNode(entries);
    m_currentLeaf->m_fatherNode = m_root;

    m_root->addPointer(m_currentLeaf);

    LeafNode* lastOverflowNode = NULL;
    bool overflow = false;

    for(int i = 0; i < size; ) {
        
        if(m_currentLeaf->isFull()) {
            // current leaf is full, so we have to create a new one
            LeafNode* newLeaf = new LeafNode(entries + i);

            if(m_currentLeaf->m_fatherNode->isFull()) {
                // there is no space left, so we have to split the father node recursively to be able to link the new leaf
                m_currentLeaf->m_fatherNode = m_currentLeaf->m_fatherNode->split(m_root);
            }

            // there is space left, so we can simply add the new leaf there, but only if it is not an overflow leaf
            if(data[i] != (i > 0 ? data[i-1] : -1)) {
                m_currentLeaf->m_fatherNode->addKey(data[i]);
                m_currentLeaf->m_fatherNode->addPointer(newLeaf);

                newLeaf->m_fatherNode = m_currentLeaf->m_fatherNode;
                if(overflow) {
                    newLeaf->setPrevious(lastOverflowNode);
                    lastOverflowNode->setNext(newLeaf);
                    overflow = false;
                }
                else {
                    newLeaf->setPrevious(m_currentLeaf);
                    m_currentLeaf->setNext(newLeaf);
                }

                m_currentLeaf = newLeaf;
            }
            else {
                // we have an overflow due to duplicate keys, so fill up overflow leaves until
                // a different key arrives or all keys have been processed
                overflow = true;
                newLeaf->setAsOverflowNode();
                lastOverflowNode = newLeaf;
                newLeaf->m_fatherNode = NULL;
                newLeaf->setPrevious(m_currentLeaf);
                m_currentLeaf->setNext(newLeaf);
                while(i < size && data[i] == (i > 0 ? data[i-1] : -1)) {
                    if(newLeaf->isFull()) {
                        LeafNode* oldOverflow = newLeaf;
                        newLeaf = new LeafNode(entries + i, true);
                        lastOverflowNode = newLeaf;
                        newLeaf->m_fatherNode = NULL;
                        newLeaf->setPrevious(oldOverflow);
                        oldOverflow->setNext(newLeaf);
                    }
                    newLeaf->addKey(data[i]);
                    newLeaf->addRowId(i);
                    ++i;
                }
                // the overflow nodes have been filled up, so get back to normal
            }
        }

        // space left in leaf, so simply insert. need to check on node size, i should jump from node size!!
        if(!overflow) {
            m_currentLeaf->addKey(data[i]);
            m_currentLeaf->addRowId(i);
            ++i;
        }
    }
}

rowId_t BulkBPTree::lookup(const colKey_t& key) {
    const LeafNode* leaf = m_root->lookup(key);
    return leaf->getRowId(key);
}

rowId_t BulkBPTree::gte(const colKey_t& key) {
    const LeafNode* leaf = m_root->lookup(key);

    return leaf->getGTE(key);
}

rowId_t BulkBPTree::lt(const colKey_t& key) {
    const LeafNode* leaf = m_root->lookup(key);

    return leaf->getLT(key);
}

rowId_t BulkBPTree::lte(const colKey_t& key) {
    const LeafNode* leaf = m_root->lookup(key);

    return leaf->getLTE(key);
}



void *build_bptree_bulk(IndexEntry *c, int64_t n) {
    return new BulkBPTree(c, n);
}

void *build_bptree_bulk_int(int64_t* c, int64_t n) {
    return new BulkBPTree(c, n);
}


int64_t BPNode::m_maxNumberOfEntries = 0;
