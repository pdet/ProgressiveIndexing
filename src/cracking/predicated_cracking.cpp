#include "../include/cracking/predicated_cracking.h"
#include <cstdlib>
#include <assert.h>

int64_t crackInTwoPredicated(IndexEntry *&c,int64_t begin, int64_t end,const int64_t pivot )
{
    struct {
        unsigned which;
        int64_t values[2];
        int64_t tuples[2];

    } localbuffer[2];

    localbuffer[0].which = 0;
    localbuffer[0].values[0] = c[begin].m_key;
    localbuffer[0].tuples[0] = c[begin].m_rowId;
    localbuffer[0].values[1] = c[end].m_key;
    localbuffer[0].tuples[1] = c[end].m_rowId;

    localbuffer[1].which = 1;
    localbuffer[1].values[0] = c[begin].m_key;
    localbuffer[1].tuples[0] = c[begin].m_rowId;
    localbuffer[1].values[1] = c[end].m_key;
    localbuffer[1].tuples[1] = c[end].m_rowId;

    unsigned i = 0;
    while (begin <= end) {
        const int64_t value = localbuffer[i].values[localbuffer[i].which];
        const int64_t row_id = localbuffer[i].tuples[localbuffer[i].which];

        c[begin].m_key = c[end].m_key = value;
        c[begin].m_rowId = c[end].m_rowId = row_id;
        const unsigned advance_lower  = value <  pivot;
        const unsigned advance_higher = value >= pivot;
        begin += advance_lower;
        end   -= advance_higher;
        localbuffer[i].which = advance_higher;
        localbuffer[i].values[0] =  c[begin].m_key;
        localbuffer[i].tuples[0] =  c[begin].m_rowId;
        localbuffer[i].values[1] =  c[end].m_key;
        localbuffer[i].tuples[1] =  c[end].m_rowId;
        i = 1 - i;
    }
    if(begin < 0)
        return 0;
    while (begin > 0 && c[begin].m_key >= pivot)
        begin--;
    return begin;
}

void checkColumn(IndexEntry *&c, int64_t pos, int64_t pivot, int64_t column_size){
    for (size_t i = 0; i <column_size; i ++ ){
        if (i <= pos){
            if (c[i].m_key >= pivot){
                fprintf(stderr,"Error 1\n");
                assert(0);
            }
        }
        else
            if(c[i].m_key <  pivot){
                fprintf(stderr,"Error 2\n");
                assert(0);
            }
    }
}


AvlTree predicatedCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey) {

    IntPair p1, p2;

    p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
    IntPair pivot_pair = NULL;
    pivot_pair = (IntPair) malloc(sizeof(struct int_pair));
    pivot_pair->first = crackInTwoPredicated(c, p1->first, p1->second, lowKey);
    T = Insert(pivot_pair->first, lowKey, T);
    p2 = FindNeighborsLT(highKey, T, dataSize - 1);
    pivot_pair->second = crackInTwoPredicated(c, p2->first, p2->second, highKey);
    T = Insert(pivot_pair->second, highKey, T);
    free(p1);
    free(p2);
    if (pivot_pair) {
        free(pivot_pair);
        pivot_pair = NULL;
    }
    return T;
}
