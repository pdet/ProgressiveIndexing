#pragma once

#include <cstdint>
#include <limits>
#include <memory>

//! From stackoverflow nice templated function to static_cast unique_ptr
//! conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
template <typename TO, typename FROM> std::unique_ptr<TO> static_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
    return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
}

class QSAVLNode {
  public:
    int64_t pivot;

    //! Where we are on sorting this node
    //! Eventually current_start == current_end and tha'' the offset for
    //! this pivot
    size_t current_start;
    size_t current_end;
    //! If we finish sorting this piece
    bool sorted;
    //! The children
    std::unique_ptr<QSAVLNode> left;
    std::unique_ptr<QSAVLNode> right;
    void setLeft(std::unique_ptr<QSAVLNode> child);
    void setRight(std::unique_ptr<QSAVLNode> child);

    QSAVLNode(int64_t pivot, size_t column_size) : pivot(pivot), current_start(0), current_end(column_size - 1), sorted(false),left(nullptr),right(nullptr) {}

    QSAVLNode(int64_t pivot, size_t current_start, size_t current_end)
        : pivot(pivot), current_start(current_start), current_end(current_end), sorted(false),left(nullptr),right(nullptr) {}
    bool noChildren();
};

class intPair {
  public:
    int64_t value;
    size_t offset;

    intPair(int64_t value, size_t offset) : offset(offset), value(value){};
};

class QSAVLTree {
  public:
    QSAVLTree() : root(nullptr){};
    QSAVLTree(std::unique_ptr<QSAVLNode> root, size_t columnSize) : root(move(root)), columnSize(columnSize){};
    //! Root of the tree
    std::unique_ptr<QSAVLNode> root;
    size_t columnSize;
    //! Check if root doesn't have children (i.e., we are in the creation
    //! phase)
    bool noChildren();
    QSAVLNode* inOrderSucessor(QSAVLNode* node);
    QSAVLNode* FindNodeLT(int64_t X);
    QSAVLNode* FindNodeGTE(int64_t X);
    QSAVLNode* FindMin(QSAVLNode* T);
    QSAVLNode* FindMax(QSAVLNode* T);
    std::vector<QSAVLNode*> GetNodesInOrder();
    QSAVLNode* mergeChildren(QSAVLNode* P);
    QSAVLNode* findParent(QSAVLNode* P);
    QSAVLNode* inOrderPredecessor(QSAVLNode* node);
    std::unique_ptr<intPair> pieceStart(QSAVLNode* node);
    std::unique_ptr<intPair> pieceEnd(QSAVLNode* node);
    size_t getHeight(QSAVLNode* node);
};
