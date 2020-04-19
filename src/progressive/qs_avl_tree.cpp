#include "progressive/qs_avl_tree.hpp"
#include <algorithm>
#include <cassert>
#include <vector>
using namespace std;

bool QSAVLTree::noChildren() {
    auto root_conv = (QSAVLNode*)root.get();
    return !(root_conv->left.get() || root_conv->right.get());
}

QSAVLNode* QSAVLTree::findParent(QSAVLNode* P) {
    QSAVLNode* current_node = root.get();
    QSAVLNode* parent = current_node;
    while (current_node && current_node != P) {
        if (P->pivot > current_node->pivot) {
            parent = current_node;
            current_node = current_node->right.get();
            assert(current_node);
        } else if (P->pivot < current_node->pivot) {
            parent = current_node;
            current_node = current_node->left.get();
            assert(current_node);
        }
    }
    return parent;
}

QSAVLNode* QSAVLTree::mergeChildren(QSAVLNode* P) {
    //! If both children are sorted
    if (P->left->sorted && P->right->sorted) {
        P->sorted = true;
        P->left.reset();
        P->right.reset();

        if (P == this->root.get()) {
            return P;
        }
        auto parent = findParent(P);
        mergeChildren(parent);
    }
    return P;
}

void QSAVLNode::setLeft(std::unique_ptr<QSAVLNode> child) {
    assert(!this->left);
    assert(child->pivot < this->pivot);
    this->left = move(child);
}
void QSAVLNode::setRight(std::unique_ptr<QSAVLNode> child) {
    assert(!this->right);
    assert(child->pivot > this->pivot);
    this->right = move(child);
}

bool QSAVLNode::noChildren() { return !(left.get() || right.get()); }

QSAVLNode* QSAVLTree::FindNodeGTE(int64_t X) {
    QSAVLNode* first = 0;
    auto T = root.get();
    while (T != NULL) {
        if (X < T->pivot) {
            T = T->left.get();
        } else if (X > T->pivot) {
            first = T;
            T = T->right.get();
        } else {
            //! this is the only difference from FindNodeLT !
            first = T;
            break;
        }
    }
    return first;
}

QSAVLNode* QSAVLTree::FindNodeLT(int64_t X) {
    QSAVLNode* node = 0;
    auto T = root.get();
    while (T != NULL) {
        if (X < T->pivot) {
            node = T;
            T = T->left.get();
        } else if (X > T->pivot) {
            T = T->right.get();
        } else {
            node = T;
            break;
        }
    }
    return node;
}

QSAVLNode* QSAVLTree::FindMin(QSAVLNode* T) {
    if (T == NULL) {
        return NULL;

    } else if (T->left == NULL) {
        return T;

    } else {
        return FindMin(T->left.get());
    }
}

QSAVLNode* QSAVLTree::FindMax(QSAVLNode* T) {
    if (T != NULL) {
        while (T->right != NULL) {
            T = T->right.get();
        }
    }
    return T;
}

QSAVLNode* QSAVLTree::inOrderSucessor(QSAVLNode* node) {
    //! if we have a right node we just need to get the min of that node
    if (node->right.get()) {
        return FindMin(node->right.get());
    }
    QSAVLNode* succ = nullptr;
    auto currentNode = root.get();
    //! Start from root and search for the sucessor down the tree
    while (currentNode) {
        if (node->pivot < currentNode->pivot) {
            succ = currentNode;
            currentNode = currentNode->left.get();
        } else if (node->pivot > currentNode->pivot) {
            currentNode = currentNode->right.get();
        } else {
            break;
        }
    }
    return succ;
}

QSAVLNode* QSAVLTree::inOrderPredecessor(QSAVLNode* node) {
    //! if we have a left node we just need to get the max of that node
    if (node->left != nullptr) {
        return FindMax(node->left.get());
    }
    QSAVLNode* succ = nullptr;
    auto currentNode = root.get();
    //! Start from root and search for the predecessor down the tree
    while (currentNode) {
        if (currentNode->pivot > node->pivot) {
            currentNode = currentNode->left.get();
        } else if (currentNode->pivot < node->pivot) {
            succ = currentNode;
            currentNode = currentNode->right.get();
        } else {
            break;
        }
    }
    return succ;
}

std::unique_ptr<intPair> QSAVLTree::pieceStart(QSAVLNode* node) {
    auto predecessor = inOrderPredecessor(node);
    if (predecessor) {
        return move(make_unique<intPair>(predecessor->pivot, predecessor->current_start));
    } else {
        return move(make_unique<intPair>(0, 0));
    }
}
std::unique_ptr<intPair> QSAVLTree::pieceEnd(QSAVLNode* node) {
    auto sucessor = inOrderSucessor(node);
    if (sucessor) {
        return move(make_unique<intPair>(sucessor->pivot, sucessor->current_end));
    } else {
        return move(make_unique<intPair>(columnSize, columnSize - 1));
    }
};

void _GetNodesInOrder(QSAVLNode* T, std::vector<QSAVLNode*>& vector) {
    if (T->left) {
        _GetNodesInOrder(T->left.get(), vector);
    }
    vector.push_back(T);
    if (T->right) {
        _GetNodesInOrder(T->right.get(), vector);
    }
}

vector<QSAVLNode*> QSAVLTree::GetNodesInOrder() {
    std::vector<QSAVLNode*> nodesOrder;
    _GetNodesInOrder(root.get(), nodesOrder);
    return nodesOrder;
}

size_t QSAVLTree::getHeight(QSAVLNode* node) {
    if (!node->left.get()) {
        return 1;
    }
    return 1 + std::max(getHeight(node->left.get()), getHeight(node->right.get()));
}
