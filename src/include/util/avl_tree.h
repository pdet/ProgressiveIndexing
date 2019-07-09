#ifndef PROGRESSIVEINDEXING_AVLTREE_H
#define PROGRESSIVEINDEXING_AVLTREE_H


#include <cstdlib>
#include "../structs.h"

struct AvlNode;

typedef struct AvlNode *PositionAVL;
typedef struct AvlNode *AvlTree;

AvlTree MakeEmpty(AvlTree T);

int64_t FindLT(ElementType X, AvlTree T);

int64_t FindLTE(ElementType X, AvlTree T, ElementType limit);

PositionAVL FindMin(AvlTree T);

AvlTree Insert(int64_t offset, ElementType X, AvlTree T);

IntPair FindNeighborsLT(ElementType X, AvlTree T, ElementType limit);

IntPair FindNeighborsGTE(ElementType X, AvlTree T, ElementType limit);

int64_t lookup(ElementType X, AvlTree T);

AvlTree FindNeighborsLT(ElementType X, AvlTree T);

AvlTree FindNeighborsGTFinal(ElementType X, AvlTree T);

std::vector<AvlTree> GetNodesInOrder(AvlTree T);

bool checkAVLConvergence(AvlTree T);

AvlTree FindFirstPiece(AvlTree T);

AvlTree FindLastPiece(AvlTree T);

AvlTree FindNeighborsLTFinal(ElementType X, AvlTree T);

AvlTree FindNodeLTE(ElementType X, AvlTree T);
// AvlTree FindNodeLTE( ElementType X, AvlTree T, ElementType limit );

void Print(AvlTree T);


#endif
