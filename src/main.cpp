#include <vector>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <ctime>
#include <ios>
#include <iomanip>
#include <climits>
#include <cassert>
#include <cmath>
#include <string>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <queue>
#include <math.h>
#include "include/structs.h"
#include "include/util/file_manager.h"
#include "include/util/binary_search.h"
#include "include/full_index/hybrid_radix_insert_sort.h"
#include "include/full_index/bulkloading_bp_tree.h"
#include "include/cracking/standard_cracking.h"
#include "include/cracking/stochastic_cracking.h"
#include "include/cracking/progressive_stochastic_cracking.h"
#include "include/progressive/incremental.h"
#include "include/generate/random.h"
#include "include/cracking/predicated_cracking.h"
#include "include/cracking/rewired_cracking.h"
#include "include/util/rewiring.h"

#pragma clang diagnostic ignored "-Wformat"

string COLUMN_FILE_PATH, QUERIES_FILE_PATH, ANSWER_FILE_PATH;
int64_t COLUMN_SIZE, NUM_QUERIES, L2_SIZE;
int ALGORITHM, RUN_CORRECTNESS, INTERACTIVITY_IS_PERCENTAGE, RADIXSORT_TOTAL_BYTES, DECAY_QUERIES, DECAY_TYPE;
double ALLOWED_SWAPS_PERCENTAGE;
int64_t num_partitions = 0;
double DELTA, INTERACTIVITY_THRESHOLD;
TotalTime query_times;
size_t current_query;
constexpr std::size_t CHUNK_SIZE = 2lu * 1024lu * 1024lu; // 2 MiB
const bool UseHugepages = true;
//rewiring::memory_file<true> &memfile{"hugepages_cracking_AoS"};

typedef ResultStruct (*progressive_function)(Column &c, int64_t low, int64_t high, double delta);

typedef double (*estimate_function)(Column &c, int64_t low, int64_t high, double delta);

void full_scan(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    double prefix_sum = 0;
    double time = 0;
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        if (current_query == 0) {
            start = chrono::system_clock::now();
            int64_t sum = 0;
            for (size_t j = 0; j < COLUMN_SIZE; j++)
                if (column.data[j] >= rangeQueries.leftpredicate[current_query] &&
                    column.data[j] <= rangeQueries.rightpredicate[current_query])
                    sum += column.data[j];
            end = chrono::system_clock::now();
            time = chrono::duration<double>(end - start).count();
            if (sum != answers[current_query])
                fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                        answers[current_query], sum);
        }
        query_times.q_time[current_query].query_processing += time;
        prefix_sum += time;
        query_times.prefix_sum[current_query] += prefix_sum;

    }
}

int64_t scanQuery(IndexEntry *c, int64_t from, int64_t to) {
    int64_t sum = 0;
    for (int64_t i = from; i <= to; i++) {
        sum += c[i].m_key;
    }

    return sum;
}

void *fullIndex(IndexEntry *c) {
    hybrid_radixsort_insert(c, COLUMN_SIZE);
    void *I = build_bptree_bulk(c, COLUMN_SIZE);
    return I;
}

void full_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *data = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        data[i].m_key = column.data[i];
        data[i].m_rowId = i;
    }


    BulkBPTree *T = (BulkBPTree *) fullIndex(data);
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        // query
        start = chrono::system_clock::now();
        int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
        int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
        int64_t sum = scanQuery(data, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                    answers[current_query], sum);
    }
    free(data);
    free(T);
}

void standard_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;


    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                             rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        //Querying
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], sum);
    }
    free(crackercolumn);
}

void predicated_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;


    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();
        T = predicatedCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                             rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        //Querying
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], sum);
    }
    free(crackercolumn);
}


int64_t * crackInTwoRewired(const int64_t pivot, int64_t *begin, int64_t *end,
                            rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &data,
                            rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &vm_buffer)
{
    assert(vm_buffer.size == 4 * CHUNK_SIZE && "buffer of wrong size");
    using namespace rewiring;
    constexpr std::size_t ELEMENTS_PER_CHUNK = CHUNK_SIZE / sizeof(int64_t);

    int64_t * const p_buffer = static_cast<int64_t*>(vm_buffer.addr);
    int64_t * const p_buffer_lo_begin = p_buffer + 0 * ELEMENTS_PER_CHUNK;
    int64_t * const p_buffer_hi_end = p_buffer + 4 * ELEMENTS_PER_CHUNK;

    enum {
        BUFFER_LO_FIRST  = 0,
        BUFFER_LO_SECOND = 1,
        BUFFER_HI_SECOND = 2,
        BUFFER_HI_FIRST  = 3,
    };

    /* Define pointers. */
    int64_t *p_buf_lo = p_buffer_lo_begin;
    int64_t *p_buf_hi = p_buffer_hi_end;

    int64_t * const p_data = static_cast<int64_t*>(data.addr);

    const std::size_t chunk_lo_begin = (begin - p_data) / ELEMENTS_PER_CHUNK;
    const std::size_t chunk_hi_end   = (end - p_data - 1) / ELEMENTS_PER_CHUNK;
    assert(p_data + chunk_lo_begin * ELEMENTS_PER_CHUNK <= begin);
    assert(end <= p_data + (chunk_hi_end + 1) * ELEMENTS_PER_CHUNK);
    assert(p_data + chunk_hi_end * ELEMENTS_PER_CHUNK < end);
    std::size_t next_chunk_lo = chunk_lo_begin;
    std::size_t next_chunk_hi = chunk_hi_end;
    assert(p_data + next_chunk_lo * ELEMENTS_PER_CHUNK <= begin);
    assert(p_data + (next_chunk_lo + 1) * ELEMENTS_PER_CHUNK > begin);
    assert(p_data + next_chunk_hi * ELEMENTS_PER_CHUNK < end);
    assert(p_data + (next_chunk_hi + 1) * ELEMENTS_PER_CHUNK >= end);

    int64_t *p_src_lo = p_data + next_chunk_lo * ELEMENTS_PER_CHUNK;
    int64_t *p_src_hi = p_data + next_chunk_hi * ELEMENTS_PER_CHUNK;

    auto store = [](void *dest, int64_t &value) {
        *((int64_t*) dest) = value;
//            }
    };
    auto insert = [&](int64_t &value) {
        if (value < pivot)
            *p_buf_lo++ = value;
        else
            *--p_buf_hi = value;

    };

    bool isFirst;
    bool fromLo;

    {
        /* memcpy portions outside [begin,end) to the buffers */
        int64_t * const start_lo = p_data + next_chunk_lo * ELEMENTS_PER_CHUNK;
        int64_t * const end_hi = p_data + (next_chunk_hi + 1) * ELEMENTS_PER_CHUNK;
        assert(begin - start_lo < long(ELEMENTS_PER_CHUNK));
        assert(end_hi - end < long(ELEMENTS_PER_CHUNK));

        if (start_lo != begin) {
            for (auto p = start_lo; p != begin; ++p) {
                assert(*p < pivot);
                store(p_buf_lo++, *p);
            }
        }
        if (end_hi != end) {
            for (auto p = end_hi; p != end; --p) {
                assert(p[-1] >= pivot);
                store(--p_buf_hi, p[-1]);
            }
        }

        /* partition the rest of the pages */
        if (next_chunk_lo == next_chunk_hi) {
            for (auto p = begin; p != end; ++p) {
                insert(*p);
            }
            p_src_lo += ELEMENTS_PER_CHUNK;
        } else {
            if (start_lo != begin) {
                for (auto p = begin; p != start_lo + ELEMENTS_PER_CHUNK; ++p) {
                    insert(*p);
                }
                p_src_lo += ELEMENTS_PER_CHUNK;
            }
            if (end_hi != end) {
                for (auto p = end_hi - ELEMENTS_PER_CHUNK; p != end; ++p) {
                    insert(*p);
                }
                p_src_hi -= ELEMENTS_PER_CHUNK;
            }
        }


        /* If one page of a buffer has been filled, swap the virtual memory region of this buffer into the virtual
         * memory region of the input data. */
        const bool is_full_lo = p_buf_lo - p_buffer_lo_begin >= long(ELEMENTS_PER_CHUNK);
        const bool is_full_hi = p_buffer_hi_end - p_buf_hi >= long(ELEMENTS_PER_CHUNK);
        if (is_full_lo) {
            swap(vm_buffer, BUFFER_LO_FIRST, data, next_chunk_lo);
            swap(vm_buffer, BUFFER_LO_FIRST, vm_buffer, BUFFER_LO_SECOND);
            p_buf_lo -= ELEMENTS_PER_CHUNK;
            ++next_chunk_lo;
        }
        if (is_full_hi) {
            swap(vm_buffer, BUFFER_HI_FIRST, data, next_chunk_hi);
            swap(vm_buffer, BUFFER_HI_FIRST, vm_buffer, BUFFER_HI_SECOND);
            p_buf_hi += ELEMENTS_PER_CHUNK;
            --next_chunk_hi;
        }


        if (is_full_lo and is_full_hi) {
            isFirst = true;
            fromLo = true;
        } else if (is_full_lo) {
            isFirst = false;
            fromLo = true;
        } else if (is_full_hi) {
            isFirst = false;
            fromLo = false;
        } else {
            isFirst = start_lo == begin and end_hi == end;
            fromLo = start_lo == begin;
        }
    }

    /* Repeatedly partiton a chunk at a time into the buffers.  Rewire buffers if they run full. */
    while (p_src_lo <= p_src_hi) {
        int64_t *p_src = fromLo ? p_src_lo : p_src_hi;
        assert((p_src - p_data) % ELEMENTS_PER_CHUNK == 0);

        /* Partition one chunk from the input data into buffer_lo/buffer_hi. */
        for (const int64_t *end = p_src + ELEMENTS_PER_CHUNK; p_src != end; ++p_src)
            insert(*p_src);


        /* Update pointers. */
        if (not fromLo)
            p_src -= 2lu * ELEMENTS_PER_CHUNK;
        (fromLo ? p_src_lo : p_src_hi) = p_src;

        /* If one page of a buffer has been filled, swap the virtual memory region of this buffer into the virtual
         * memory region of the input data. */
        const bool is_full_lo = p_buf_lo - p_buffer_lo_begin >= long(ELEMENTS_PER_CHUNK);
        const bool is_full_hi = p_buffer_hi_end - p_buf_hi >= long(ELEMENTS_PER_CHUNK);
        if (is_full_lo) {
            assert(p_src_lo >= p_data + (next_chunk_lo + 1) * ELEMENTS_PER_CHUNK);
            swap(vm_buffer, BUFFER_LO_FIRST, data, next_chunk_lo);
            swap(vm_buffer, BUFFER_LO_FIRST, vm_buffer, BUFFER_LO_SECOND);
            p_buf_lo -= ELEMENTS_PER_CHUNK;
            ++next_chunk_lo;
        }
        if (is_full_hi) {
            assert(p_src_hi + ELEMENTS_PER_CHUNK <= p_data + next_chunk_hi * ELEMENTS_PER_CHUNK);
            swap(vm_buffer, BUFFER_HI_FIRST, data, next_chunk_hi);
            swap(vm_buffer, BUFFER_HI_FIRST, vm_buffer, BUFFER_HI_SECOND);
            p_buf_hi += ELEMENTS_PER_CHUNK;
            --next_chunk_hi;
        }


        if (isFirst and not is_full_lo and not is_full_hi) {
            isFirst = false;
            fromLo = not fromLo;
        } else if (is_full_lo and is_full_hi) {
            isFirst = true;
            fromLo = true;
        } else if (is_full_lo)
            fromLo = true;
        else if (is_full_hi)
            fromLo = false;
    }

    const std::size_t num_elements_buffer_lo = p_buf_lo - p_buffer_lo_begin;
    const std::size_t num_elements_buffer_hi = p_buffer_hi_end - p_buf_hi;

    assert(num_elements_buffer_lo < ELEMENTS_PER_CHUNK);
    assert(num_elements_buffer_hi < ELEMENTS_PER_CHUNK);

    std::size_t next_chunk = fromLo ? next_chunk_lo : next_chunk_hi;
    int64_t *partition = p_data + next_chunk * ELEMENTS_PER_CHUNK + num_elements_buffer_lo;
    assert(num_elements_buffer_lo != 0 or num_elements_buffer_hi == 0);
    if (num_elements_buffer_lo != 0 or num_elements_buffer_hi != 0) {
        assert(begin <= partition);
        assert(partition <= end);

        /* Merge buffer_hi into buffer_lo.  This will exactly fill the first page of buffer_lo. */
        assert(num_elements_buffer_lo + num_elements_buffer_hi == ELEMENTS_PER_CHUNK);
        std::memcpy(p_buf_lo, p_buf_hi, sizeof(int64_t) * num_elements_buffer_hi);
//        assert(verify_partition(p_buffer_lo_begin, p_buf_lo, p_buffer_lo_begin + ELEMENTS_PER_CHUNK));
        assert(*p_buf_lo >= pivot);
        assert(*(p_buf_lo - 1) < pivot);

        /* Swap the first page of buffer_lo into the virtual address space of the data. */
        assert(vm_buffer.addr == (void*) p_buffer_lo_begin);
        swap(vm_buffer, BUFFER_LO_FIRST, data, next_chunk);

        assert(*partition >= pivot);
        assert(*(partition - 1) < pivot);

    }

    return partition;
}


//AvlTree rewiredCracking(IndexEntry *&c, int dataSize, AvlTree T, int lowKey, int highKey) {
AvlTree rewiredCracking(rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &data, rewiring::memory_mapping<UseHugepages, CHUNK_SIZE> &vm_buffer, int dataSize, AvlTree T, int lowKey, int highKey) {

    IntPair p1, p2;

    p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
    IntPair pivot_pair = NULL;
    pivot_pair = (IntPair) malloc(sizeof(struct int_pair));
    crackInTwoRewired(lowKey, &p1->first, &p1->second, data, vm_buffer);
//    pivot_pair->first = crackInTwoPredicated(c, p1->first, p1->second, lowKey);
    T = Insert(pivot_pair->first, lowKey, T);
    p2 = FindNeighborsLT(highKey, T, dataSize - 1);
//    pivot_pair->second = crackInTwoPredicated(c, p2->first, p2->second, highKey);
    T = Insert(pivot_pair->second, highKey, T);
    free(p1);
    free(p2);
    if (pivot_pair) {
        free(pivot_pair);
        pivot_pair = NULL;
    }
    return T;
}

void rewired_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    start = chrono::system_clock::now();
    rewiring::memory_file<true> memfile{"hugepages_cracking_AoS"};
//    rewiring::memory_file<true> &memfile{"hugepages_cracking_AoS"};
    memfile.resize(COLUMN_SIZE * sizeof(crackercolumn));
    auto vm_copy = memfile.map<2lu * 1024lu * 1024lu>(COLUMN_SIZE  * sizeof(crackercolumn), 0);
    auto copy = vm_copy.addr;
    memcpy(copy, crackercolumn, COLUMN_SIZE * sizeof(crackercolumn));

    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    auto vm_buffer = memfile.template map<2lu * 1024lu * 1024lu>(4 * 2lu * 1024lu * 1024lu, memfile.size);

    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();
        T = rewiredCracking(vm_copy, vm_buffer, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query] + 1);
//        T = rewiredCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
//                               rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        //Querying
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], sum);
    }
    free(crackercolumn);
}

int64_t queryStochastic(IndexEntry *crackercolumn, int limit) {
    int offset1 = 0;
    int offset2 = limit;
    return scanQuery(crackercolumn, offset1, offset2);

}

void stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;


    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;

        //Partitioning Column and Inserting in Cracker Indexing
        start = chrono::system_clock::now();

        T = stochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query] + 1, qo);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        start = chrono::system_clock::now();

        //Querying
        if (qo->view1) {
            qo->sum += queryStochastic(qo->view1, qo->view_size1 - 1);
        }
        if (qo->middlePart_size > 0) {
            qo->sum += queryStochastic(qo->middlePart, qo->middlePart_size - 1);
        }
        if (qo->view2) {
            qo->sum += queryStochastic(qo->view2, qo->view_size2 - 1);
        }
        if (qo->view1) {
            free(qo->view1);
            qo->view1 = NULL;
        }
        if (qo->view2) {
            free(qo->view2);
            qo->view2 = NULL;
        }
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();

        if (qo->sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], qo->sum);
    }
    free(crackercolumn);
}

void progressive_stochastic_cracking(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    //Intializing Query Output
    QueryOutput *qo = (QueryOutput *) malloc(sizeof(struct QueryOutput));
    qo->sum = 0;
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        qo->view1 = NULL;
        qo->view_size1 = 0;
        qo->view2 = NULL;
        qo->view_size2 = 0;
        qo->middlePart = NULL;
        qo->middlePart_size = 0;
        qo->sum = 0;
        start = chrono::system_clock::now();
        //Partitioning Column and Inserting in Cracker Indexing
        T = progressiveStochasticCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                                          rangeQueries.rightpredicate[current_query] + 1, qo);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();
        start = chrono::system_clock::now();

        //Querying
        if (qo->view1) {
            qo->sum += queryStochastic(qo->view1, qo->view_size1 - 1);
        }
        if (qo->middlePart_size > 0) {
            qo->sum += queryStochastic(qo->middlePart, qo->middlePart_size - 1);
        }
        if (qo->view2) {
            qo->sum += queryStochastic(qo->view2, qo->view_size2 - 1);
        }
        if (qo->view1) {
            free(qo->view1);
            qo->view1 = NULL;
        }
        if (qo->view2) {
            free(qo->view2);
            qo->view2 = NULL;
        }
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (qo->sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], qo->sum);

    }
    free(crackercolumn);

}

void generate_equal_size_partitions_order(vector<int64_t> *partitions, int64_t min, int64_t max) {
    int64_t medium = (max + min) / 2;
    partitions->push_back(medium);
    num_partitions++;
    if (num_partitions > 999)
        return;
    generate_equal_size_partitions_order(partitions, min, medium);
    generate_equal_size_partitions_order(partitions, medium, max);

}


void coarse_granular_index(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers) {
    chrono::time_point<chrono::system_clock> start, middle, end;
    chrono::duration<double> elapsed_seconds;
    start = chrono::system_clock::now();
    vector<int64_t> partitions;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    // Running 1000 - equal sized partitions
    generate_equal_size_partitions_order(&partitions, 0, COLUMN_SIZE);
    for (size_t i = 0; i < 1000; i++) {
        T = standardCracking(crackercolumn, COLUMN_SIZE, T, partitions[i], partitions[i]);
    }

    end = chrono::system_clock::now();
    query_times.idx_time[0].index_creation += chrono::duration<double>(end - start).count();

    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        // crack
        start = chrono::system_clock::now();

        T = standardCracking(crackercolumn, COLUMN_SIZE, T, rangeQueries.leftpredicate[current_query],
                             rangeQueries.rightpredicate[current_query] + 1);
        end = chrono::system_clock::now();
        query_times.idx_time[current_query].index_creation += chrono::duration<double>(end - start).count();

        // query
        start = chrono::system_clock::now();
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[current_query], (AvlTree) T, COLUMN_SIZE - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[current_query] + 1, (AvlTree) T, COLUMN_SIZE - 1);
        int64_t offset1 = p1->first;
        int64_t offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %zu\n Expected : %ld    Got : %ld \n", current_query,
                    answers[current_query], sum);
    }
    free(crackercolumn);
}

int64_t scanQuery(int64_t *c, int64_t from, int64_t to) {
    int64_t sum = 0;
    for (int64_t i = from; i <= to; i++) {
        sum += c[i];
    }

    return sum;
}

void *fullIndex(int64_t *c, int n) {
    void *I = build_bptree_bulk_int(c, n);
    return I;
}

void progressive_indexing(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                          vector<double> &deltas, progressive_function function) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    if (function == range_query_incremental_bucketsort_equiheight) {
        // create the initial buckets for bucketsort without counting the time
        // we do this because it throws off our benchmarking otherwise
        // otherwise we spent one iteration on determining min,max of the column
        // which is unrelated to the delta of the algorithm or anything else
        // we assume the database knows these from statistics in the paper
        // so we just run a "warm up run" to determine these here prior to benchmarking
        function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
    }

    if (function == range_query_incremental_radixsort_msd ||
        function == range_query_incremental_bucketsort_equiheight) {
        // initialize the sort index
        column.sortindex.resize(column.data.size());
        column.bucket_index.final_index = new int64_t[column.data.size()];
        column.bucket_index.final_index_entries = 0;
    }

    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        ResultStruct results;


        if (column.converged) {
            start = chrono::system_clock::now();
            int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
            int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
            sum = scanQuery(column.final_data, offset1, offset2);
            end = chrono::system_clock::now();
            query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
        } else {
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query], 0);
            end = chrono::system_clock::now();
            double baseline = chrono::duration<double>(end - start).count();
            query_times.q_time[current_query].query_processing += baseline;
            if (results.sum != answers[current_query])
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                        answers[current_query], sum);
            start = chrono::system_clock::now();

            results = function(column, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query], DELTA);
            end = chrono::system_clock::now();

            sum = results.sum;
            query_times.idx_time[current_query].index_creation +=
                    chrono::duration<double>(end - start).count() - baseline;
        }

        if (sum != answers[current_query])
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                    answers[current_query], sum);
        if (!converged && column.converged) {
            converged = true;
            T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());

        }
        deltas[current_query] += DELTA;
    }
}

double calcute_current_interactivity_threshold_exponential(double initial_query_time, double ratio,
                                                           size_t current_query) {
    return initial_query_time * pow((1 - ratio), current_query);
}

double calcute_current_interactivity_threshold_linear(double initial_query_time, double ratio, size_t current_query) {
    return initial_query_time - ratio * current_query;
}

double calcute_exponential_decay_ratio(double full_scan_time, double full_index_time) {
    return 1 - pow(full_index_time / full_scan_time, 1.0 / DECAY_QUERIES);
}

double calculate_linear_decay_ratio(double full_scan_time, double full_index_time) {
    return -((full_index_time - full_scan_time) / DECAY_QUERIES);
}


void progressive_indexing_cost_model(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                                     vector<double> &deltas, progressive_function function,
                                     estimate_function estimate) {
    chrono::time_point<chrono::system_clock> start, end;
    BulkBPTree *T;
    int64_t sum = 0;
    bool converged = false;
    if (function == range_query_incremental_bucketsort_equiheight) {
        // create the initial buckets for bucketsort without counting the time
        // we do this because it throws off our benchmarking otherwise
        // otherwise we spent one iteration on determining min,max of the column
        // which is unrelated to the delta of the algorithm or anything else
        // we assume the database knows these from statistics in the paper
        // so we just run a "warm up run" to determine these here prior to benchmarking
        function(column, rangeQueries.leftpredicate[0], rangeQueries.rightpredicate[0], DELTA);
    }

    if (function == range_query_incremental_radixsort_msd ||
        function == range_query_incremental_bucketsort_equiheight) {
        // initialize the sort index
        column.sortindex.resize(column.data.size());
        column.bucket_index.final_index = new int64_t[column.data.size()];
        column.bucket_index.final_index_entries = 0;
    }
    ResultStruct results;

    // 0.5s
    // Run Dummy Full Scan to check if its higher or lower than the interactivity threshold
    // first get the base time with delta = 0
    start = chrono::system_clock::now();
    results = function(column, rangeQueries.leftpredicate[0],
                       rangeQueries.rightpredicate[0], 0);
    end = chrono::system_clock::now();
    if (results.sum != answers[0]) {
        fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", 0, answers[0], sum);
    }
    double full_scan_time = chrono::duration<double>(end - start).count();
    double best_convergence_delta_time = 1.5 * full_scan_time;
    double ratio, initial_query_time, final_query_time;
    double prefix_sum = 0;

    if (INTERACTIVITY_IS_PERCENTAGE)
        INTERACTIVITY_THRESHOLD = INTERACTIVITY_THRESHOLD * full_scan_time;
    for (current_query = 0; current_query < NUM_QUERIES; current_query++) {
        if (full_scan_time > INTERACTIVITY_THRESHOLD) {
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query], 0);
            end = chrono::system_clock::now();
            if (results.sum != answers[current_query]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                        answers[current_query], sum);
            }
            double base_time = chrono::duration<double>(end - start).count();
            query_times.q_time[current_query].query_processing += base_time;
            double estimated_time = 0;
            size_t ITERATIONS = 10;
            double estimated_delta = 0.5;
            double offset = estimated_delta / 2;
            for (size_t j = 0; j < ITERATIONS; j++) {
                estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
                                          rangeQueries.rightpredicate[current_query], estimated_delta);
                if (estimated_time > best_convergence_delta_time) {
                    estimated_delta -= offset;
                } else {
                    estimated_delta += offset;
                }
                offset /= 2;
            }
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query],
                               rangeQueries.rightpredicate[current_query], estimated_delta);
            end = chrono::system_clock::now();
            query_times.idx_time[current_query].index_creation +=
                    chrono::duration<double>(end - start).count() - base_time;
            sum = results.sum;
            double time = chrono::duration<double>(end - start).count();
            prefix_sum += chrono::duration<double>(end - start).count();
            query_times.prefix_sum[current_query] += prefix_sum;
            if (sum != answers[current_query]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                        answers[current_query], sum);
            }
            // now interpolate the real delta
            double cost_per_delta = (time - base_time) / estimated_delta;
            double real_delta = (best_convergence_delta_time - base_time) / cost_per_delta;
            if (column.converged) {
                converged = true;
                T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
            }
            deltas[current_query] += real_delta;
            start = chrono::system_clock::now();
            results = function(column, rangeQueries.leftpredicate[current_query + 1],
                               rangeQueries.rightpredicate[current_query + 1], 0);
            end = chrono::system_clock::now();
            if (results.sum != answers[current_query + 1]) {
                fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                        answers[current_query + 1], sum);
            }
            full_scan_time = chrono::duration<double>(end - start).count();
        } else {
            if (converged) {
                start = chrono::system_clock::now();
                int64_t offset1 = (T)->gte(rangeQueries.leftpredicate[current_query]);
                int64_t offset2 = (T)->lte(rangeQueries.rightpredicate[current_query]);
                sum = scanQuery(column.final_data, offset1, offset2);
                end = chrono::system_clock::now();
                query_times.q_time[current_query].query_processing += chrono::duration<double>(end - start).count();
                prefix_sum += chrono::duration<double>(end - start).count();
                query_times.prefix_sum[current_query] += prefix_sum;
            } else {
                double estimated_time = 0;
                size_t ITERATIONS = 10;
                double estimated_delta = 0.5;
                double offset = estimated_delta / 2;
                for (size_t j = 0; j < ITERATIONS; j++) {
                    estimated_time = estimate(column, rangeQueries.leftpredicate[current_query],
                                              rangeQueries.rightpredicate[current_query], estimated_delta);
                    if (estimated_time > INTERACTIVITY_THRESHOLD) {
                        estimated_delta -= offset;
                    } else {
                        estimated_delta += offset;
                    }
                    offset /= 2;
                }
                // first get the base time with delta = 0
                start = chrono::system_clock::now();
                results = function(column, rangeQueries.leftpredicate[current_query],
                                   rangeQueries.rightpredicate[current_query], 0);
                end = chrono::system_clock::now();
                if (results.sum != answers[current_query]) {
                    fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                            answers[current_query], sum);
                }
                double base_time = chrono::duration<double>(end - start).count();
                query_times.q_time[current_query].query_processing += base_time;

                start = chrono::system_clock::now();
                results = function(column, rangeQueries.leftpredicate[current_query],
                                   rangeQueries.rightpredicate[current_query], estimated_delta);
                end = chrono::system_clock::now();
                query_times.idx_time[current_query].index_creation +=
                        chrono::duration<double>(end - start).count() - base_time;
                sum = results.sum;
                double time = chrono::duration<double>(end - start).count();
                prefix_sum += chrono::duration<double>(end - start).count();
                query_times.prefix_sum[current_query] += prefix_sum;
                if (sum != answers[current_query]) {
                    fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", current_query,
                            answers[current_query], sum);
                }
                // now interpolate the real delta
                double cost_per_delta = (time - base_time) / estimated_delta;
                double real_delta = (INTERACTIVITY_THRESHOLD - base_time) / cost_per_delta;
                if (column.converged) {
                    converged = true;
                    T = (BulkBPTree *) fullIndex(column.final_data, column.data.size());
                }
                deltas[current_query] += real_delta;
                if (DECAY_QUERIES && current_query == 0) {
                    initial_query_time = time;
                    final_query_time = 0.002; // Educated Guess, We assume the DBMS has statistics about this
                    if (DECAY_TYPE == 0)
                        ratio = calculate_linear_decay_ratio(initial_query_time, final_query_time);
                    else
                        ratio = calcute_exponential_decay_ratio(initial_query_time, final_query_time);
                }

                if (current_query < DECAY_QUERIES) {
                    if (DECAY_TYPE == 0)
                        INTERACTIVITY_THRESHOLD = calcute_current_interactivity_threshold_linear(initial_query_time,
                                                                                                 ratio,
                                                                                                 current_query + 1);
                    else
                        INTERACTIVITY_THRESHOLD = calcute_current_interactivity_threshold_exponential(
                                initial_query_time,
                                ratio,
                                current_query + 1);
                }
                query_times.cost_model[current_query] += estimated_time;
            }

        }
    }
    column.Clear();
}


void print_help(int argc, char **argv) {
    fprintf(stderr, "Unrecognized command line option.\n");
    fprintf(stderr, "Usage: %s [args]\n", argv[0]);
    fprintf(stderr, "   --column-path\n");
    fprintf(stderr, "   --query-path\n");
    fprintf(stderr, "   --answers-path\n");
    fprintf(stderr, "   --num-queries\n");
    fprintf(stderr, "   --column-size\n");
    fprintf(stderr, "   --algorithm\n");
    fprintf(stderr, "   --progressive-cracking-swap\n");
    fprintf(stderr, "   --delta\n");
    fprintf(stderr, "   --correctness\n");
    fprintf(stderr, "   --interactivity-threshold\n");
    fprintf(stderr, "   --interactivity-is-percentage\n");
    fprintf(stderr, "   --decay-queries\n");
    fprintf(stderr, "   --decay-type\n");


}

pair<string, string> split_once(string delimited, char delimiter) {
    auto pos = delimited.find_first_of(delimiter);
    return {delimited.substr(0, pos), delimited.substr(pos + 1)};
}

int main(int argc, char **argv) {
    COLUMN_FILE_PATH = "generated_data/10000000/column";
    QUERIES_FILE_PATH = "generated_data/10000000/query_1_2";
    ANSWER_FILE_PATH = "generated_data/10000000/answer_1_2";
    L2_SIZE = 16000; // 320000 64 bit integers
    ALLOWED_SWAPS_PERCENTAGE = 0.1;
    NUM_QUERIES = 10000;
    COLUMN_SIZE = 10000000;
    ALGORITHM = 0;
    DELTA = 0.1;
    RUN_CORRECTNESS = 0; // By Default run regular runs
    INTERACTIVITY_IS_PERCENTAGE = 1; // By default interactivity is a percentage of FS time
    INTERACTIVITY_THRESHOLD = 0.5;
    DECAY_QUERIES = 0;
    DECAY_TYPE = 0;
    int repetition = 1;
    for (int i = 1; i < argc; i++) {
        auto arg = string(argv[i]);
        if (arg.substr(0, 2) != "--") {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
        arg = arg.substr(2);
        auto p = split_once(arg, '=');
        auto &arg_name = p.first;
        auto &arg_value = p.second;
        if (arg_name == "column-path") {
            COLUMN_FILE_PATH = arg_value;
        } else if (arg_name == "query-path") {
            QUERIES_FILE_PATH = arg_value;
        } else if (arg_name == "answer-path") {
            ANSWER_FILE_PATH = arg_value;
        } else if (arg_name == "num-queries") {
            NUM_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "column-size") {
            COLUMN_SIZE = atoi(arg_value.c_str());
        } else if (arg_name == "algorithm") {
            ALGORITHM = atoi(arg_value.c_str());
        } else if (arg_name == "progressive-cracking-swap") {
            ALLOWED_SWAPS_PERCENTAGE = atof(arg_value.c_str());
        } else if (arg_name == "delta") {
            DELTA = atof(arg_value.c_str());
        } else if (arg_name == "correctness") {
            RUN_CORRECTNESS = atoi(arg_value.c_str());
        } else if (arg_name == "interactivity-is-percentage") {
            INTERACTIVITY_IS_PERCENTAGE = atoi(arg_value.c_str());
        } else if (arg_name == "interactivity-threshold") {
            INTERACTIVITY_THRESHOLD = atof(arg_value.c_str());
        } else if (arg_name == "decay-queries") {
            DECAY_QUERIES = atoi(arg_value.c_str());
        } else if (arg_name == "decay-type") {
            DECAY_TYPE = atoi(arg_value.c_str());
        } else {
            print_help(argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    RADIXSORT_TOTAL_BYTES = ceil(log2(COLUMN_SIZE));
    RangeQuery rangequeries;
    load_queries(&rangequeries, QUERIES_FILE_PATH, NUM_QUERIES);
    Column c;
    load_column(&c, COLUMN_FILE_PATH, COLUMN_SIZE);

    vector<int64_t> answers;
    load_answers(&answers, ANSWER_FILE_PATH, NUM_QUERIES);
    if (!RUN_CORRECTNESS)
        repetition = 10;
    query_times.Initialize(NUM_QUERIES);
    vector<double> times(NUM_QUERIES);
    vector<double> deltas(NUM_QUERIES);
    current_query = 0;
    for (size_t i = 0; i < repetition; i++) {
        switch (ALGORITHM) {
            case 1:
                full_scan(c, rangequeries, answers);
                break;
            case 2:
                full_index(c, rangequeries, answers);
                break;
            case 3:
                standard_cracking(c, rangequeries, answers);
                break;
            case 4:
                stochastic_cracking(c, rangequeries, answers);
                break;
            case 5:
                progressive_stochastic_cracking(c, rangequeries, answers);
                break;
            case 6:
                coarse_granular_index(c, rangequeries, answers);
                break;
            case 7:
                progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_quicksort);
                break;
            case 8:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_quicksort, get_estimated_time_quicksort);
                break;
            case 9:
                progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_bucketsort_equiheight);
                break;
            case 10:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_bucketsort_equiheight,
                                                get_estimated_time_bucketsort);
                break;
            case 11:
                progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_lsd);
                break;
            case 12:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_radixsort_lsd,
                                                get_estimated_time_radixsort_lsd);
                break;
            case 13:
                progressive_indexing(c, rangequeries, answers, deltas, range_query_incremental_radixsort_msd);
                break;
            case 14:
                progressive_indexing_cost_model(c, rangequeries, answers, deltas,
                                                range_query_incremental_radixsort_msd,
                                                get_estimated_time_radixsort_msd);
                break;
            case 15:
                predicated_cracking(c, rangequeries, answers);
                break;
            case 16:
                rewired_cracking(c, rangequeries, answers);
                break;
        }
    }
    if (!RUN_CORRECTNESS)
        for (size_t i = 0; i < NUM_QUERIES; i++) {
            double total_time, total_indexing, total_querying;
            total_time = query_times.idx_time[i].index_creation + query_times.q_time[i].query_processing;
            cout << deltas[i] / repetition << ";" << query_times.q_time[i].query_processing / repetition << ";" <<
                 query_times.idx_time[i].index_creation / repetition << ";" << total_time / repetition << ";"
                 << query_times.prefix_sum[i] / repetition << ";" << query_times.cost_model[i] / repetition << "\n";
        }
}
