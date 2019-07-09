/* 
 * File:   index.h
 * Author: laurent
 *
 * Created on July 4, 2015, 2:48 PM
 * 
 * This file contains type and function declarations relating to indexes.
 */

#ifndef CRACKER_INDEX_H
#define    CRACKER_INDEX_H

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../structs.h"

// C interfaces
typedef void *crackerIndex_pt;
typedef void *workingArea_pt;

extern "C" crackerIndex_pt getNewCrackerIndex();
void deleteCrackerIndex(crackerIndex_pt crackerIndex);
extern "C" void update_or_insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" void insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" void update(entry_t e, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" working_area_t findNeighborsLT(entry_t e, crackerIndex_pt index, offset_t offsetLimit);
bool converged(crackerIndex_pt index);
#endif	/* CRACKER_INDEX_H */

