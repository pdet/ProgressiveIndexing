#include "../include/sort/sort.hpp"

#include <cstring>

#define INSERT_SORT_LEVEL 64

//! insertion sort
void insertion_sort(IdxColEntry* c, int n) {
    int k;
    for (k = 1; k < n; k++) {
        IdxColEntry key = c[k];
        int i = k - 1;
        while ((i >= 0) && (key < c[i])) {
            c[i + 1] = c[i];
            i--;
        }
        c[i + 1] = key;
    }
}

//! hybrid radix sort: switches to insertion sort after a threshold
void do_hybrid_radixsort_insert(IdxColEntry* c, unsigned int n, int shift) {
    unsigned int last[256], ptr[256], cnt[256];
    unsigned int i, j, k, sorted, remain;
    IdxColEntry temp, swap;

    memset(cnt, 0, 256 * sizeof(unsigned int)); //! Zero counters
    switch (shift) {                            //! Count occurrences
    case 0:
        for (i = 0; i < n; i++)
            cnt[c[i].m_key & 0xFF]++;
        break;
    case 8:
        for (i = 0; i < n; i++)
            cnt[(c[i].m_key >> 8) & 0xFF]++;
        break;
    case 16:
        for (i = 0; i < n; i++)
            cnt[(c[i].m_key >> 16) & 0xFF]++;
        break;
    case 24:
        for (i = 0; i < n; i++)
            cnt[c[i].m_key >> 24]++;
        break;
        /*
         * 	Note that even though our radix sort implementations
         * work on 8 byte keys, only 4 bytes are actually processed, as
         * our key domain is never lager than [0, 2^32-1]. In a system
         * implementation, this would be handled before sorting by
         * inspecting the max pivot of the column in the statistics.
         */
    }
    sorted = (cnt[0] == n); //! Accumulate counters into pointers
    ptr[0] = 0;
    last[0] = cnt[0];
    for (i = 1; i < 256; i++) {
        last[i] = (ptr[i] = last[i - 1]) + cnt[i];
        sorted |= (cnt[i] == n);
    }
    if (!sorted) { //! Go through all swaps
        i = 255;
        remain = n;
        while (remain > 0) {
            while (ptr[i] == last[i])
                i--;    //! Find uncompleted pivot range
            j = ptr[i]; //! Grab first element in cycle
            swap = c[j];
            k = (swap.m_key >> shift) & 0xFF;
            if (i != k) { //! Swap into correct range until cycle
                          //! completed
                do {
                    temp = c[ptr[k]];
                    c[ptr[k]++] = swap;
                    k = ((swap = temp).m_key >> shift) & 0xFF;
                    remain--;
                } while (i != k);
                c[j] = swap; //! Place last element in cycle
            }
            ptr[k]++;
            remain--;
        }
    }
    if (shift > 0) { //! Sort on next digit
        shift -= 8;
        for (i = 0; i < 256; i++) {
            if (cnt[i] > INSERT_SORT_LEVEL)
                do_hybrid_radixsort_insert(&c[last[i] - cnt[i]], cnt[i], shift);
            else if (cnt[i] > 1)
                insertion_sort(&c[last[i] - cnt[i]], cnt[i]);
        }
    }
}

void hybrid_radixsort_insert(IdxColEntry* c, int n) { do_hybrid_radixsort_insert(c, n, 24); }

//! insertion sort
static void insertion_sort(int64_t* val, size_t* ind, int n) {
    int k;
    for (k = 1; k < n; k++) {
        int64_t key = val[k];
        size_t index = ind[k];
        int i = k - 1;
        while ((i >= 0) && (key < val[i])) {
            val[i + 1] = val[i];
            ind[i + 1] = ind[i];
            i--;
        }
        val[i + 1] = key;
        ind[i + 1] = index;
    }
}

//! hybrid radix sort: switches to insertion sort after a threshold
static void do_hybrid_radixsort_insert(int64_t* val, size_t* ind, unsigned int n, int shift) {
    unsigned int last[256], ptr[256], cnt[256];
    unsigned int i, j, k, sorted, remain;
    int64_t temp, swap;
    size_t temp_i, swap_i;

    memset(cnt, 0, 256 * sizeof(unsigned int)); //! Zero counters
    switch (shift) {                            //! Count occurrences
    case 0:
        for (i = 0; i < n; i++)
            cnt[val[i] & 0xFF]++;
        break;
    case 8:
        for (i = 0; i < n; i++)
            cnt[(val[i] >> 8) & 0xFF]++;
        break;
    case 16:
        for (i = 0; i < n; i++)
            cnt[(val[i] >> 16) & 0xFF]++;
        break;
    case 24:
        for (i = 0; i < n; i++)
            cnt[val[i] >> 24]++;
        break;
        /*
         * 	Note that even though our radix sort implementations
         * work on 8 byte keys, only 4 bytes are actually processed, as
         * our key domain is never lager than [0, 2^32-1]. In a system
         * implementation, this would be handled before sorting by
         * inspecting the max pivot of the column in the statistics.
         */
    }
    sorted = (cnt[0] == n); //! Accumulate counters into pointers
    ptr[0] = 0;
    last[0] = cnt[0];
    for (i = 1; i < 256; i++) {
        last[i] = (ptr[i] = last[i - 1]) + cnt[i];
        sorted |= (cnt[i] == n);
    }
    if (!sorted) { //! Go through all swaps
        i = 255;
        remain = n;
        while (remain > 0) {
            while (ptr[i] == last[i])
                i--;    //! Find uncompleted pivot range
            j = ptr[i]; //! Grab first element in cycle
            swap = val[j];
            swap_i = ind[j];
            k = (swap >> shift) & 0xFF;
            if (i != k) { //! Swap into correct range until cycle
                          //! completed
                do {
                    temp = val[ptr[k]];
                    temp_i = ind[ptr[k]];
                    ind[ptr[k]] = swap_i;
                    val[ptr[k]++] = swap;
                    swap = temp;
                    swap_i = temp_i;
                    k = (swap >> shift) & 0xFF;
                    remain--;
                } while (i != k);
                val[j] = swap; //! Place last element in cycle
                ind[j] = swap_i;
            }
            ptr[k]++;
            remain--;
        }
    }
    if (shift > 0) { //! Sort on next digit
        shift -= 8;
        for (i = 0; i < 256; i++) {
            if (cnt[i] > INSERT_SORT_LEVEL)
                do_hybrid_radixsort_insert(&val[last[i] - cnt[i]], &ind[last[i] - cnt[i]], cnt[i], shift);
            else if (cnt[i] > 1)
                insertion_sort(&val[last[i] - cnt[i]], &ind[last[i] - cnt[i]], cnt[i]);
        }
    }
}

bool itqs(int64_t* val, size_t* ind, size_t n) {
    do_hybrid_radixsort_insert(val, ind, n, 24);
    return true;
}
