/* 
 * File:   index.cpp
 * Author: laurent
 *
 * Created on July 6, 2015, 5:12 PM
 * 
 * This file contains all functions relating to index accesses.
 * 
 */

#include "../include/cracking/index.h"
//#include "../includes/utils.h"

// creates an instance of index
crackerIndex_pt getNewCrackerIndex() {
    return new map_t;
}

void deleteCrackerIndex(crackerIndex_pt crackerIndex) {
    if(crackerIndex) {
        cracker_index_t* ci = static_cast<cracker_index_t*>(crackerIndex);
        delete ci;
    }
}

// inserts an entry if it does not exist, updates it if it does
void update_or_insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index) {
    cracker_index_t crackerIndex = (cracker_index_t) index;
    cracker_index_iterator_t it = crackerIndex->find(e);

    if (e == MIN_KEY) {
        it = crackerIndex->begin();
    }

    // only update if entry key is actually found
    if (it != crackerIndex->end()) {
        it->second.entryLength = length;
        it->second.sorted = sorted;
    } else {
        // insert
        insert(e, offset, length, sorted, index);
    }
    return;
}

// updates an entry in the index
//void update(entry_t e, offset_t length, bool sorted, crackerIndex_pt index) {
//    cracker_index_t crackerIndex = (cracker_index_t) index;
//    cracker_index_iterator_t it = crackerIndex->find(e);
//
//    if (e == MIN_KEY) {
//        it = crackerIndex->begin();
//    }
//
//    // only update if entry key is actually found
//    if (it != crackerIndex->end()) {
//        it->second.entryLength = length;
//        it->second.sorted = sorted;
//    } else {
//        debug("Ignoring invalid entry update on key: %lu\n", e);
//    }
//    return;
//}

// inserts an entry into the index
void insert(entry_t e, offset_t offset, offset_t length, bool sorted, crackerIndex_pt index) {
    cracker_index_t crackerIndex = (cracker_index_t) index;
    offset_length_t offsetWithLength;
    offsetWithLength.offset = offset;
    offsetWithLength.entryLength = length;
    offsetWithLength.sorted = sorted;
    std::pair<cracker_index_iterator_t, bool> res = crackerIndex->insert(std::make_pair(e, offsetWithLength));
    if (!res.second) {
        res.first->second.offset = offset;
        res.first->second.entryLength = length;
        res.first->second.sorted = sorted;
    }
}

bool converged(crackerIndex_pt index){
    bool converged = false;
    cracker_index_t crackerIndex = (cracker_index_t) index;
    cracker_index_iterator_t it = crackerIndex->lower_bound(0);
    while (it != crackerIndex->end()){
        if (!it->second.sorted || (it->second.entryLength == sizeof(entry_t)*8))
            return false;
        it++;
    }
    return true;
}


// finds the entries that are closest to e [*,*)
working_area_t findNeighborsLT(entry_t e, crackerIndex_pt index, offset_t offsetLimit) {
    cracker_index_t crackerIndex = (cracker_index_t) index;

    // check if index was already initialized
    if (crackerIndex->size() <= 0) {
        insert(MIN_KEY, -1, 0, false, index);
        insert(MAX_KEY + 1, offsetLimit, sizeof(entry_t) * 8, true, index);
    }

    // init result container
    working_area_t result;

    // find right neighbor to e
    cracker_index_iterator_t it = crackerIndex->lower_bound(e);

    // if exact match found go one to the right
    if (it->first == e) {
        it++;
    }

    // copy values of right neighbor
    offset_length_t owlH;
    owlH.offset = it->second.offset;
    owlH.entryLength = it->second.entryLength;
    owlH.sorted = it->second.sorted;
    // handle right border case
    if (it->first >= MAX_KEY) {
        owlH.offset = offsetLimit;
    }

    // build right entry
    entry_offset_pair_t tmpH;
    tmpH.entry = it->first;
    tmpH.offsetWithLength = owlH;
    result.second = tmpH;

    // find left neighbor to e
    it--;

    // copy values of left neighbor
    offset_length_t owlL;
    owlL.offset = it->second.offset + 1;
    owlL.entryLength = it->second.entryLength;
    owlL.sorted = it->second.sorted;
    // handle left border case
    if (it->first <= MIN_KEY) {
        owlL.offset = 0;
    }
    // build left entry
    entry_offset_pair_t tmpL;
    tmpL.entry = it->first;
    tmpL.offsetWithLength = owlL;
    result.first = tmpL;

    return result;
}
