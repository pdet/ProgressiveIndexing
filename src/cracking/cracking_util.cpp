#include <sys/mman.h>
#include "../include/cracking/cracking_util.h"

int numSwapps = 0;

void* malloc_huge(size_t size) {
    void* p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
    if(!p) {
        printf("Out of memory\n");
        exit(1);
    }

#ifndef __MACH__
    madvise(p, size, MADV_HUGEPAGE);
   	   madvise(p, size, MADV_SEQUENTIAL);
#endif

    return p;
}

void exchange(IndexEntry*& c, int64_t x1, int64_t x2){
    IndexEntry tmp = *(c+x1);
    *(c+x1) = *(c+x2);
    *(c+x2) = tmp;
    numSwapps++;
}

int crackInTwoItemWise(IndexEntry*& c, int64_t posL, int64_t posH, int64_t med){
    int x1 = posL, x2 = posH;
    while (x1 <= x2) {
        if(c[x1] < med)
            x1++;
        else {
            while (x2 >= x1 && (c[x2] >= med))
                x2--;
            if(x1 < x2){
                exchange(c, x1,x2);
                x1++;
                x2--;
            }
        }
    }
    if(x1 < x2)
        printf("Not all elements were inspected!");
    x1--;
    if(x1 < 0)
        x1 = 0;
    return x1;
}

IntPair crackInThreeItemWise(IndexEntry *c, int64_t posL, int64_t posH, int64_t low, int64_t high){
    int x1 = posL, x2 = posH;
    while(x2 > x1 && c[x2] >= high)
        x2--;
    int x3 = x2;
    while(x3 > x1 && c[x3] >= low){
        if(c[x3]>=high){
            exchange(c, x2, x3);
            x2--;
        }
        x3--;
    }
    while(x1<x3){
        if(c[x1] < low)
            x1++;
        else{
            exchange(c, x1, x3);
            while(x3 > x1 && c[x3] >= low){
                if(c[x3]>=high){
                    exchange(c, x2, x3);
                    x2--;
                }
                x3--;
            }
        }
    }
    IntPair p = (IntPair) malloc(sizeof(struct int_pair));
    p->first = x3;
    p->second = x2;
    return p;
}
