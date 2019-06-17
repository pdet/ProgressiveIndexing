//
// Created by Pedro Holanda on 03/05/19.
//

#pragma once

#ifndef EXPERIMENTS_H
#define    EXPERIMENTS_H

#include <cstdlib>
#include <unistd.h>

// QUERY WORKLOADS FROM STOCHASTIC CRACKING
#define RANDOM          0
#define SKEW            1
#define SEQRANDOM       2
#define SEQZOOMIN       3
#define PERIODIC        4
#define ZOOMIN          5
#define SEQUENTIAL      6
#define ZOOMOUTALT      7
#define ZOOMINALT       8
#define EVEN            9

// DATA WORKLOADS
#define UNIFORM         0
#define NORMAL          1
#define ZIPF            2

// KEY DOMAIN (determines data size)
#define MIN_KEY         0
#define MAX_KEY         18446744073709551614UL      // max unsigned 64 bit - 1

// RANDOM SEED
#define FIXED_SEED      133713371337

// OTHER EXPERIMENTAL SETTINGS
#define INSERT_SORT_LEVEL 64
#define STREAM_UNIT     2
#define NO_SPLIT_MARK   -1
#define CRACKED_MARK    666
#define KEY_COLUMN_ID   0
#define NUM_COLUMNS     1
#define NUM_REPETITIONS 1
#define NUM_DUPLICATES  1
#define ZIPF_ALPHA      0.6f
#define ZIPF_MAX        1000
#define JUMP_FACTOR     1000
//#define PRINT_WORKLOADS
//#define SIMULATED_ANNEALING
//#define FILE_INPUT

// QUERY WORKLOAD SETTINGS
#ifndef DATA_WORKLOAD
#define DATA_WORKLOAD   UNIFORM
#endif
#ifndef QUERY_WORKLOAD
#define QUERY_WORKLOAD  RANDOM
#endif
// HYBRID CRACK SORT SETTINGS
#define CHUNKS          1024

// ALPACAS SETTINGS

#ifndef NUM_BITS_MAX
#define NUM_BITS_MAX    6        // max #bits used in radix partitioning (def: 8)
#endif
#ifndef NUM_BITS_MIN
#define NUM_BITS_MIN    3        // min #bits used in radix partitioning (def: 5)
#endif
#ifndef NUM_BITS
#define NUM_BITS        NUM_BITS_MIN
#endif
#ifndef NUM_OOP_BITS
#define NUM_OOP_BITS    10
#endif
#ifndef SCALING_THRESHOLD
#define SCALING_THRESHOLD ELEMENTS_IN_TLB
#endif
#ifndef TIME_LIMIT_MAX
#define TIME_LIMIT_MAX  5.0
#endif
#ifndef RECURSION_THRESHOLD
#define RECURSION_THRESHOLD ELEMENTS_IN_L1
#endif
#ifndef SORTING_THRESHOLD
#define SORTING_THRESHOLD ELEMENTS_IN_L2
#endif
#ifndef TOLERANCE
#define TOLERANCE       5.0f;
#endif
#ifndef FANOUT
#define FANOUT FLINEAR
#endif
#ifndef BUFFER_SIZE
#define BUFFER_SIZE     16
#endif

//#ifndef LINK_COND_1
//#define LINK_COND_1     !isFitInL1
//#endif
//#ifndef LINK_COND_2
//#define LINK_COND_2     isEmpty
//#endif
#ifndef MAX_RECURSION_COUNT
#define MAX_RECURSION_COUNT 3
#endif

// SINGLE TEST MACRO
//#define SINGLE_EXP exp_ALL_IN_ONE

#endif	/* EXPERIMENTS_H */
