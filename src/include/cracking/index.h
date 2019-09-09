#pragma once

#include "../structs.h"

#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>

//! C interfaces
typedef void *crackerIndex_pt;
typedef void *workingArea_pt;

extern "C" crackerIndex_pt getNewCrackerIndex();
void deleteCrackerIndex(crackerIndex_pt crackerIndex);
extern "C" void update_or_insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" void insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" void update(entry_t e, offset_t length, bool sorted, crackerIndex_pt index);
extern "C" working_area_t findNeighborsLT(entry_t e, crackerIndex_pt index, offset_t offsetLimit);
bool converged_adaptive(crackerIndex_pt index);
