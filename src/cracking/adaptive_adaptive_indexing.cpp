#include "../include/cracking/adaptive_adaptive_indexing.h"
#include "emmintrin.h"
#include "immintrin.h"

void copy_key_column(row_t *src, crow_t *dst, offset_t size, entry_t &max) {
    max = src[0].cols[KEY_COLUMN_ID];
    for (offset_t i = 0; i < size; i++) {
        dst[i].rowId = src[i].rowId;
        dst[i].col = src[i].cols[KEY_COLUMN_ID];
        if (max < dst[i].col) {
            max = dst[i].col;
        }
    }
}

// f linear
offset_t FLINEAR(const working_area_t p, const offset_t numBits, const offset_t bMin, const offset_t bMax, const offset_t sThreshold) {
    const double pSize = p.second.offsetWithLength.offset-p.first.offsetWithLength.offset+1;
    const double val = std::min(1.0, pSize/sThreshold);
    return bMin + (offset_t)ceil((bMax - bMin) * (1.0-val));
}

void init_workingData(wdPartitioned_t *const workingData, offset_t size) {

    crow_t *dst = NULL;

    int s = posix_memalign((void **) &dst, L1_LINE_SIZE, sizeof(*dst) * size);
    if (s != 0) {
        exit(s);
    }
    workingData->dst = dst;

    // initialize cracker columns to pre-fault the pages (not measured)
    memset(dst, 0, sizeof(*dst) * size);

    // copy data from input to cracker column
    entry_t max = 0;
    copy_key_column(workingData->src, dst, size, max);
    workingData->maxKey = max;

}

offset_t findInitialShiftOffset(entry_t key) {
    offset_t shifts = 0;
    while (key > 0) {
        key >>= 1;
        shifts++;
    }
    // number of bits used by key - number of shifts
    shifts = (offset_t) (8 * sizeof(entry_t)) - shifts;
    return shifts;
}

entry_t scan(crow_t *dst, offset_t from, offset_t to) {
    entry_t res = 0;
    for (offset_t i = from; i <= to; ++i) {
        res += dst[i].col;
    }
    return res;
}

entry_t filter(crow_t *dst, offset_t from, offset_t to, entry_t lowKey, entry_t highKey) {
    entry_t res = 0;
    for (offset_t i = from; i <= to; ++i) {
        if (lowKey <= dst[i].col && dst[i].col < highKey) {
            res += dst[i].col;
        }
    }
    return res;
}


entry_t FILTER_helper(crow_t *dst, offset_t size, entry_t keyL, entry_t keyH, crackerIndex_pt index) {

    // find partition start and end offsets
    entry_t res;
    working_area_t p1, p2;
    offset_t offL, offH, offML, offMH;
    p1 = findNeighborsLT(keyL, index, size - 1);
    p2 = findNeighborsLT(keyH, index, size - 1);
    offL = p1.first.offsetWithLength.offset;
    offML = p1.second.offsetWithLength.offset;
    offMH = p2.first.offsetWithLength.offset;
    offH = p2.second.offsetWithLength.offset;

    // check if start and end offsets are different
    if (p1.first.entry == p2.first.entry) {
        // if same, then filter once
        res = filter(dst, offL, offML, keyL, keyH);

    } else {
        // else do filter, scan, filter
        res = filter(dst, offL, offML, keyL, keyH);
        res += scan(dst, offML + 1, offMH - 1);
        res += filter(dst, offMH, offH, keyL, keyH);

    }

    return res;
}

offset_t bin_search(entry_t key, crow_t *dst, offset_t oL, offset_t oH) {
    // find leftmost entry that is still bigger
    if (oH < oL) {
        while (oH > 0 && dst[oH].col >= key) oH--;
        //return oH;
        return oH + 1;
    }
    // calculate new offset and recurse
    offset_t oM = (oH - oL) / 2 + oL;
    if (dst[oM].col < key) {
        return bin_search(key, dst, oM + 1, oH);
    } else {
        return bin_search(key, dst, oL, oM - 1);
    }
}

bool isSorted(const working_area_t &p) {
    // check if sorted
    return p.first.offsetWithLength.sorted || (p.first.offsetWithLength.entryLength == sizeof(entry_t)*8);
}

// Insertion sort
void insertion_sort(crow_t *c, offset_t n) {
    offset_t k;
    for (k = 1; k < n; ++k) {
        entry_t key = c[k].col;
        entry_t rowId = c[k].rowId;
        offset_t i = k - 1;
        while ((i >= 0) && (key < c[i].col)) {
            c[i + 1] = c[i];
            --i;
        }
        c[i + 1].col = key;
        c[i + 1].rowId = rowId;
    }
}

// Hybrid radix sort: switches to insertion sort after a threshold
void do_hybrid_radix_insert_sort(crow_t *c, offset_t n, offset_t shift) {
    const offset_t numBuckets = 256;
    const entry_t mask = 0xFF;

    offset_t last[numBuckets];
    offset_t ptr[numBuckets];
    offset_t cnt[numBuckets];

    offset_t i, j, remain;
    bool sorted;
    entry_t k;
    crow_t temp, swap;

    memset(cnt, 0, numBuckets * sizeof(offset_t)); // Zero counters
#if KEY_SIZE == 4
    switch (shift) {
        case 0: 	for(i=0;i<n;++i) ++cnt[c[i] & mask]; break;
        case 8: 	for(i=0;i<n;++i) ++cnt[(c[i] >> 8) & mask]; break;
        case 16: 	for(i=0;i<n;++i) ++cnt[(c[i] >> 16) & mask]; break;
        case 24: 	for(i=0;i<n;++i) ++cnt[c[i] >> 24]; break;
    }
#else
    switch (shift) {
        case 0:
            for (i = 0; i < n; ++i) ++cnt[c[i].col & mask];
            break;
        case 8:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 8) & mask];
            break;
        case 16:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 16) & mask];
            break;
        case 24:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 24) & mask];
            break;
        case 32:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 32) & mask];
            break;
        case 40:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 40) & mask];
            break;
        case 48:
            for (i = 0; i < n; ++i) ++cnt[(c[i].col >> 48) & mask];
            break;
        case 56:
            for (i = 0; i < n; ++i) ++cnt[c[i].col >> 56];
            break;
        default:
            for (i = 0; i < n; ++i) ++cnt[c[i].col >> 56];
            break;
    }
#endif

    sorted = (cnt[0] == n);    // Accumulate counters into pointers
    ptr[0] = 0;
    last[0] = cnt[0];
    for (i = 1; i < numBuckets; ++i) {
        last[i] = (ptr[i] = last[i - 1]) + cnt[i];
        sorted |= (cnt[i] == n);
    }
    if (!sorted) {    // Go through all swaps
        i = numBuckets - 1;
        remain = n;
        while (remain > 0) {
            while (ptr[i] == last[i])
                --i;    // Find uncompleted value range
            j = ptr[i];    // Grab first element in cycle
            swap = c[j];
            k = (swap.col >> shift) & mask;
            if (i != k) {    // Swap into correct range until cycle completed
                do {
                    temp = c[ptr[k]];
                    c[ptr[k]] = swap;
                    ++ptr[k];
                    k = ((swap = temp).col >> shift) & mask;
                    --remain;
                } while (i != k);
                c[j] = swap;    // Place last element in cycle
            }
            ++ptr[k];
            --remain;
        }
    }

    if (shift > 0) {    // Sort on next digit
        shift -= 8;
        for (i = 0; i < numBuckets; ++i) {
            if (cnt[i] > INSERT_SORT_LEVEL) {
                do_hybrid_radix_insert_sort(&c[last[i] - cnt[i]], cnt[i], shift);
            }
            else if (cnt[i] > 1) {
                insertion_sort(&c[last[i] - cnt[i]], cnt[i]);
            }
        }
    }
}

// Hybrid radix/insert sort helper
void hybrid_radix_insert_sort(crow_t *c, offset_t n) {
#if KEY_SIZE == 4
    do_hybrid_radix_insert_sort(c, n, 24);
#else
    do_hybrid_radix_insert_sort(c, n, 56);
#endif
}

// check if a partition size is below a threshold
bool isPartitionSmallerEqualsThan(const working_area_t &p, const offset_t numElements) {
    // check if partition size is below a threshold
    const bool isSmallerEquals = (p.second.offsetWithLength.offset - p.first.offsetWithLength.offset + 1) <= numElements;
    return isSmallerEquals;
}

// standard in-place radix partitioning
void inPlaceRadixPartitioning(crow_t *const c, const offset_t n, const offset_t numPartitions, offset_t *histogram,
                              const offset_t shiftOffset, const offset_t shift) {

    const offset_t numBuckets = (offset_t) numPartitions;
    const entry_t mask = 0xFFFFFFFF;

    offset_t last[numBuckets];
    offset_t ptr[numBuckets];
    offset_t cnt[numBuckets];

    offset_t i, j, remain;
    entry_t k;
    bool sorted;
    crow_t temp, swap;

    memset(cnt, 0, numBuckets * sizeof(offset_t)); // Zero counters

    if(shift == sizeof(entry_t) * 8) {
        cnt[0] = n;
    }
    else {
        for (i = 0; i < n; ++i) ++cnt[((c[i].col << shiftOffset) >> shift) & mask];
    }

    sorted = (cnt[0] == n); // Accumulate counters into pointers
    ptr[0] = 0;
    last[0] = cnt[0];
    for (i = 1; i < numBuckets; ++i) {
        last[i] = (ptr[i] = last[i - 1]) + cnt[i];
        sorted |= (cnt[i] == n);
    }
    memcpy(histogram, last, numBuckets * sizeof(offset_t));

    if (!sorted) { // Go through all swaps
        i = numBuckets - 1;
        remain = n;
        while (remain > 0) {
            while (ptr[i] == last[i])
                --i; // Find uncompleted value range
            j = ptr[i]; // Grab first element in cycle
            swap = c[j];
            k = (sizeof(entry_t) * 8 == shift) ? 0 : ((swap.col << shiftOffset) >> shift) & mask;
            if (i != k) { // Swap into correct range until cycle completed
                do {
                    temp = c[ptr[k]];
                    c[ptr[k]] = swap;
                    ++ptr[k];
                    k = (sizeof(entry_t) * 8 == shift) ? 0 : (((swap = temp).col << shiftOffset) >> shift) & mask;
                    --remain;
                } while (i != k);
                c[j] = swap; // Place last element in cycle
            }
            ++ptr[k];
            --remain;
        }
    }
}

void RADIX_helper(crow_t *dst, offset_t offset_L, offset_t offset_H, offset_t shiftOffset, crackerIndex_pt index, offset_t numBits) {

    // do in-place radix partitioning
    offset_t numPartitions = (offset_t) (1 << numBits);
    offset_t histogram[numPartitions];

    offset_t shift = (offset_t) (sizeof(entry_t) * 8 - numBits);
    inPlaceRadixPartitioning(dst + offset_L, offset_H - offset_L + 1, numPartitions, histogram, shiftOffset, shift);

    // update index
    for (offset_t j = 0; j < numPartitions; ++j) {

        // we use the shift offset, the number of bits,
        // and a key from the partition to compute the split keys
        offset_t offset_P = (j > 0 ? histogram[j - 1] : 0);
        entry_t key = dst[offset_L].col;
        key >>= sizeof(entry_t)*8 - (shiftOffset + numBits);
        key +=j;
        key <<= sizeof(entry_t)*8 - (shiftOffset + numBits);

        // update index
        update_or_insert(key, offset_L + offset_P - 1, shiftOffset + numBits, false, index);

    }
    return;
}

// decision tree for one partition key (with measurements)
working_area_t decision_tree_simple(crackerIndex_pt index, entry_t key, crow_t *dst, offset_t size, ADAPT_FUNC F,
                                    double &elapsedTime, wdPartitioned_t *const workingData) {

    // init parameters
    offset_t sComplete = workingData->sComplete;
    offset_t sRecursion = workingData->sRecursion;
    offset_t bMin = workingData->bMin;
    offset_t bMax = workingData->bMax;
    offset_t sThreshold = workingData->sThreshold;
    double tMax = workingData->tMax;
    offset_t numBits = workingData->bMin;

    // find partition

    working_area_t p = findNeighborsLT(key, index, size - 1);


    // walk through the decision tree
    if(isSorted(p)) {

        // do nothing

    } else if (isPartitionSmallerEqualsThan(p, sComplete) || (sizeof(entry_t)*8)-p.first.offsetWithLength.entryLength<=bMin) {

        hybrid_radix_insert_sort(dst + p.first.offsetWithLength.offset,
                                 p.second.offsetWithLength.offset - p.first.offsetWithLength.offset + 1);
        update_or_insert(p.first.entry, p.first.offsetWithLength.offset, p.first.offsetWithLength.entryLength, true,
                         index);


    } else  {
        double predictedTime = 0.0;
        double timePerEntry;
        // in-place radix partitioning
        // calc new partition fan-out
        numBits = F(p,numBits,bMin,bMax,sThreshold);
        // radix partition
        if(numBits > 0) {
            RADIX_helper(dst, p.first.offsetWithLength.offset, p.second.offsetWithLength.offset,
                         p.first.offsetWithLength.entryLength, index, numBits);
        }
    }
    p = findNeighborsLT(key, index, size - 1);
    return p;

}


entry_t DECISION_helper_simple(crow_t *dst, offset_t size, entry_t keyL, entry_t keyH, crackerIndex_pt index, ADAPT_FUNC F
                               , wdPartitioned_t *const workingData) {

    double elapsedTime = 0.0;
    double &eTime = elapsedTime;

    // find partitions

    working_area_t pLow = findNeighborsLT(keyL, index, size - 1);
    working_area_t pHigh = findNeighborsLT(keyH, index, size - 1);

    offset_t pLSize = pLow.second.offsetWithLength.offset-pLow.first.offsetWithLength.offset+1;
    offset_t pHSize = pHigh.second.offsetWithLength.offset-pHigh.first.offsetWithLength.offset+1;

    // process partitions
    if(pLow.first.offsetWithLength.offset == pHigh.first.offsetWithLength.offset &&
       pLow.second.offsetWithLength.offset == pHigh.second.offsetWithLength.offset) {
        // both predicates fall into same partition
        pLow = decision_tree_simple(index, keyL, dst, size, F, eTime,workingData);
        pHigh = findNeighborsLT(keyH, index, size - 1);
    }
    else {
        pLow = decision_tree_simple(index, keyL, dst, size, F, eTime,workingData);
        pHigh = decision_tree_simple(index, keyH, dst, size, F, eTime,workingData);
    }

    // answer query
    offset_t oFL, oL, oH,oFH;
    entry_t res = 0;
    // find offsets for keyL
    if(isSorted(pLow) ) {
        oFL = oL = bin_search(keyL, dst, pLow.first.offsetWithLength.offset + 1, pLow.second.offsetWithLength.offset) - 1;

    } else if (pLow.first.entry == keyL) {
        oFL = oL = pLow.first.offsetWithLength.offset;
    } else {
        // pLow needs to be filtered
        oFL =pLow.first.offsetWithLength.offset;
        oL = pLow.second.offsetWithLength.offset;
    }
    // find offsets for keyH
    if(isSorted(pHigh) ) {
        oFH = oH = bin_search(keyH, dst, pHigh.first.offsetWithLength.offset + 1, pHigh.second.offsetWithLength.offset) - 1;

    } else if (pHigh.first.entry == keyH) {
        oFH = oH = pHigh.first.offsetWithLength.offset;
    } else {
        // pHigh needs to be filtered
        oH = pHigh.first.offsetWithLength.offset;
        oFH = pHigh.second.offsetWithLength.offset;
    }
    // what to do if both keys fall into same partition
    if(pLow.first.entry == pHigh.first.entry) {
        if(oFL == oL && oFH == oH) {
            res += scan(dst, oL + 1, oH );
        } else {
            res += filter(dst, oFL, oFH, keyL, keyH );
        }

    } else {
        // else do filter, scan, filter

        res = filter(dst, oFL, oL, keyL, keyH);
        res += scan(dst, oL + 1, oH-1 );
        res += filter(dst, oH, oFH, keyL, keyH);

    }
    return res;
}

// standard in-place radix partitioning (with given histogram)
void inPlaceRadixPartitioningNoHist(crow_t * const c, const offset_t n, const offset_t numPartitions, offset_t* histogram,
                                    offset_t shiftOffset, offset_t shift) {

    const offset_t numBuckets = (offset_t)numPartitions;
    const entry_t mask = 0xFFFFFFFF;

    offset_t last[numBuckets];
    offset_t ptr[numBuckets];
    entry_t k;
    offset_t i, j, sorted, remain;
    crow_t temp, swap;

    sorted = (histogram[0] == n); // Accumulate counters into pointers
    ptr[0] = 0;
    last[0] = histogram[0];
    for (i = 1; i < numBuckets; ++i) {
        last[i] = (ptr[i] = last[i - 1]) + histogram[i];
        sorted |= (histogram[i] == n);
    }
    memcpy(histogram, last, numBuckets * sizeof (offset_t));

    if (!sorted) { // Go through all swaps
        i = numBuckets - 1;
        remain = n;
        while (remain > 0) {
            while (ptr[i] == last[i])
                --i; // Find uncompleted value range
            j = ptr[i]; // Grab first element in cycle
            swap = c[j];
            k = ((swap.col << shiftOffset) >> shift) & mask;
            if (i != k) { // Swap into correct range until cycle completed
                do {
                    temp = c[ptr[k]];
                    c[ptr[k]] = swap;
                    ++ptr[k];
                    k = (((swap = temp).col << shiftOffset) >> shift) & mask;
                    --remain;
                } while (i != k);
                c[j] = swap; // Place last element in cycle
            }
            ++ptr[k];
            --remain;
        }
    }
}

// wrapper for equi-depth radix partitioning (given histogram) (with time measurement)
void EQUI_DEPTH_RADIX_helper(const crow_t *input, crow_t *output, offset_t size, offset_t *histogram, offset_t *keys,
                             offset_t splitCount, offset_t shiftOffset, offset_t numBits, offset_t numBitsMin,
                             crackerIndex_pt index) {

    // calc partition fan-out
    const offset_t numPartitions = 1 << numBits;
    const offset_t numMinPartitions = 1 << numBitsMin;

    // ip shift settings
    const offset_t backShift2 = (offset_t)(sizeof(entry_t)*8-numBits-numBitsMin-shiftOffset);
    const offset_t my_mask2 = (offset_t)((1 << numBitsMin) - 1) << (backShift2);
    const offset_t shift2 = (offset_t)(sizeof(entry_t) * 8 - numBitsMin);

    // oop shift settings
    const offset_t backShift = (offset_t)(sizeof(entry_t)*8-numBits-shiftOffset);
    const offset_t my_mask = (offset_t)((1 << numBits) - 1) << (backShift);

    // copy initial histogram
    __attribute__((aligned(64))) offset_t *final_buckets;
    int s = posix_memalign((void **) &final_buckets, 64, numPartitions * sizeof(offset_t));
    if(s!=0) {
        exit(s);
    }
    memcpy(final_buckets,histogram, numPartitions * sizeof(offset_t));

    // accumulate histogram to pointers
    for (offset_t i = 1; i < numPartitions; ++i) {
        final_buckets[i] += final_buckets[i - 1];
    }

    // copy pointers for index inserts
    memcpy(histogram, final_buckets, numPartitions * sizeof(offset_t));

    // allocate and zero memory for in-place histograms
    offset_t equi_counters[splitCount][numMinPartitions];
    memset(equi_counters, 0, sizeof(equi_counters[0][0]) * splitCount * numMinPartitions);

    __attribute__((aligned(64))) entry_t bucket_num = 0;
    __attribute__((aligned(64))) entry_t bucket_num2 = 0;
    for (offset_t j = 0; j < size; ++j) {

        // out-of-place radix partition
        bucket_num = (input[j].col & my_mask) >> backShift;
        output[final_buckets[bucket_num] - 1] = input[j];
        --final_buckets[bucket_num];

        // piggy-back histogram building
        bucket_num2 = (input[j].col & my_mask2) >> backShift2;
        equi_counters[keys[bucket_num]][bucket_num2]++;
    }


    // update index with partitions from this step
    offset_t offset_P = 0;
    entry_t key = 0;
    offset_t newShiftOffset = shiftOffset + numBits;
    offset_t lrShift = (offset_t)(sizeof(entry_t)*8 - (newShiftOffset));
    for (offset_t j = 0; j < numPartitions; ++j) {
        offset_P = (j > 0 ? histogram[j - 1] : 0);
        key = output[0].col;
        key >>= lrShift;
        key +=j;
        key <<= lrShift;
        update_or_insert(key, offset_P - 1, newShiftOffset, false, index);
    }

    // do in-place radix partitioning on new piggy-backed histograms
    offset_t offL = 0, offH = 0, i = 0;
    newShiftOffset = shiftOffset + numBits + numBitsMin;
    lrShift = (offset_t)(sizeof(entry_t)*8 - (newShiftOffset));
    for (offset_t j = 0; j < numPartitions; j++) {

        // ignore non split partitions
        if(keys[j]==NO_SPLIT_MARK) continue;

        // else do in-place radix partitioning
        offL = (j > 0 ? histogram[j - 1] : 0);
        offH = histogram[j]-1;
        inPlaceRadixPartitioningNoHist(output + offL, offH - offL + 1, numMinPartitions, equi_counters[i], (offset_t)(shiftOffset + numBits),
                                       shift2);

        // update index with new partitions
        for (offset_t k = 0; k < numMinPartitions; ++k) {
            offset_P = (k > 0 ? equi_counters[i][k - 1] : 0);
            key = output[offL].col;
            key >>= lrShift;
            key +=k;
            key <<= lrShift;
            update_or_insert(key, offL + offset_P - 1, newShiftOffset, false, index);
        }
        i++;
    }


    free(final_buckets);

}

inline void radix_partition_with_given_histogram(crow_t *input, crow_t *output, offset_t size, offset_t *histogram,
                                          offset_t buffered_tuples, offset_t shiftOffset, offset_t numBits) {
    const entry_t mask = 0xFFFFFFFF;
    const offset_t numPartitions = (offset_t) (1 << numBits);
    const offset_t shift = (offset_t) (sizeof(entry_t) * 8 - numBits);
    __attribute__((aligned(64))) offset_t *final_buckets;
    __attribute__((aligned(64))) crow_t *buffers;
    __attribute__((aligned(64))) offset_t buffer_counters[numPartitions];

    int s = posix_memalign((void **) &final_buckets, 64, numPartitions * sizeof(offset_t));
    if (s != 0) {
        exit(s);
    }
    memset(final_buckets, 0, numPartitions * sizeof(offset_t));
    s = posix_memalign((void **) &buffers, 64, numPartitions * buffered_tuples * sizeof(crow_t));
    if (s != 0) {
        exit(s);
    }
    memset(buffer_counters, 0, numPartitions * sizeof(offset_t));

    final_buckets[0] = histogram[0];
    for (offset_t i = 1; i < numPartitions; ++i) {
        final_buckets[i] = histogram[i] + final_buckets[i - 1];
    }

    memcpy(histogram, final_buckets, numPartitions * sizeof(offset_t));

    // we have to make sure that the size of each partitioning in terms of elements is a multiple of 4
    // (since 4 entries fit into 32 bytes = 256 bits).
    // if this is not the case for a partition, we add padding elements
    for (offset_t i = 0; i < numPartitions; ++i) {
        final_buckets[i] = final_buckets[i] - final_buckets[i] % buffered_tuples;
    }

    __attribute__((aligned(64))) entry_t bucket_num = 0;
    for (offset_t j = 0; j < size; ++j) {
        bucket_num = shift == (sizeof(entry_t) * 8) ? 0 : ((input[j].col << shiftOffset) >> shift) & mask;
        entry_t offset = bucket_num * buffered_tuples;
        buffers[offset + buffer_counters[bucket_num]++] = input[j];
        if (buffer_counters[bucket_num] == buffered_tuples) {
            final_buckets[bucket_num] -= buffered_tuples;
            for (entry_t b = 0; b < buffered_tuples; b += STREAM_UNIT) {

                //TODO: Remove AVIX Intrisics
                _mm256_stream_si256(reinterpret_cast<__m256i *>(output + final_buckets[bucket_num]),
                                    _mm256_load_si256((reinterpret_cast<__m256i *>(buffers + offset + b))));
                final_buckets[bucket_num] += STREAM_UNIT;
            }
            final_buckets[bucket_num] -= buffered_tuples;
            buffer_counters[bucket_num] = 0;
        }
    }

    for (offset_t i = numPartitions - 1; i >= 0; i--) {
        offset_t offset = i * buffered_tuples;
        if (i > 0 && final_buckets[i] < histogram[i - 1]) {
            //fix the wrongly written elements
            offset_t end_partition = histogram[i] - 1;
            for (offset_t j = final_buckets[i]; j < histogram[i - 1]; j++) {
                output[end_partition--] = output[j];
            }
            final_buckets[i] = end_partition + 1;
        }
        for (entry_t b = 0; b < buffer_counters[i]; b++) {
            //rollback to end after completing the un-padded writes in the beginning
            if (final_buckets[i] <= (i > 0 ? histogram[i - 1] : 0)) {
                final_buckets[i] = histogram[i];
            }
            output[final_buckets[i] - 1] = buffers[offset + b];
            --final_buckets[i];
        }
    }

    free(final_buckets);
    free(buffers);
}

// wrapper for out-of-place radix partitioning (with given histogram) (with measurements)
void GIVEN_HIST_OOP_RADIX_helper(crow_t *in, crow_t *out, offset_t offset_L, offset_t offset_H,
                                 offset_t shiftOffset, crackerIndex_pt index, offset_t numBits,
                                 offset_t *histogram) {

    // do out-of-place radix partitioning
    offset_t numPartitions = (offset_t) (1 << numBits);
    if(numPartitions > 1) {
        radix_partition_with_given_histogram(in, out, offset_H - offset_L + 1, histogram, BUFFER_SIZE, shiftOffset,
                                             numBits);
    }
    else {
        memcpy(out, in, (offset_H - offset_L + 1) * sizeof(*out));
    }

    // update index
    if(numPartitions > 1) {
        for (offset_t j = 0; j < numPartitions; ++j) {
            offset_t offset_P = (j > 0 ? histogram[j - 1] : 0);
            entry_t key = out[offset_L].col;
            key >>= sizeof(entry_t) * 8 - (shiftOffset + numBits);
            key += j;
            key <<= sizeof(entry_t) * 8 - (shiftOffset + numBits);
            update_or_insert(key, offset_L + offset_P - 1, shiftOffset + numBits, false, index);
        }
    }

    return;
}

// wrapper for equi-depth radix partitioning (only histogram building) (with time measurement)
void EQUIDEPTHOOPRADIX_helper(crow_t *in, crow_t *out, offset_t size,
                              offset_t shiftOffset, crackerIndex_pt index, wdPartitioned_t *const workingData) {

    // init partition fan-out
    offset_t numBits = workingData->numOOPBits;
    offset_t numBitsMin = workingData->bMin;
    const offset_t initialPartitionCount = (1<<numBits);

    // init parameters for equalization (skew factor)
    offset_t tFactor = workingData->sTolerance;
    const float targetElemCount = size / (initialPartitionCount);
    const float tolerance = targetElemCount * tFactor;

    // init parameters for initial partitioning
    const offset_t shift = (offset_t)(sizeof(entry_t) * 8 - numBits);
    const entry_t mask = 0xFFFFFFFF;

    offset_t counters[initialPartitionCount];
    offset_t keys[initialPartitionCount];

    // reset histogram
    memset(counters, 0, initialPartitionCount * sizeof(offset_t));

    // build histogram
    entry_t bucket = 0;
    if(shift < sizeof(entry_t) * 8) {
        for(offset_t j = 0; j < size; ++j){
            bucket = ((in[j].col << shiftOffset) >> shift) & mask;
            ++counters[bucket];
        }
    }
    else {
        counters[0] = size;
    }

    // mark partitions that exceed targetElemCount + tolerance
    offset_t splitCount = 0;
    for(offset_t j = 0; j < initialPartitionCount; j++) {
        if(counters[j] > targetElemCount + tolerance) {
            keys[j] = splitCount;
            splitCount++;
        } else {
            keys[j] = NO_SPLIT_MARK;
        }
    }

    if(splitCount<=0) {
        // do oop radix with predetermined histogram
        GIVEN_HIST_OOP_RADIX_helper(in, out, 0, size - 1, shiftOffset, index, numBits, counters);
    } else {
        // do oop radix partitioning with further histogram building
        EQUI_DEPTH_RADIX_helper(in, out, size, counters, keys, splitCount, shiftOffset, numBits, numBitsMin, index);
    }

    return;
}
