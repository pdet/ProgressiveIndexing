#include "../include/cracking/cracking_updates.h"

extern int64_t COLUMN_SIZE_DUMMY;
void merge_ripple(IndexEntry *&column,size_t &capacity, AvlTree T, Column &updates, int64_t posL, int64_t posH, int64_t low, int64_t high) {
    int64_t remaining = posH - posL + 1;
    AvlTree lastPiece = FindLastPiece(T);

    // find the last piece from which we can swap out elements
    auto node_to_swap_out = FindNeighborsLTFinal(high, T);
    if (high >= node_to_swap_out->Element) {
        // cannot swap anything, just append
        merge(column, capacity, T, updates, posL, posH);
        updates.data.erase(updates.data.begin() + posL, updates.data.begin() + posH + 1);
        return;
    }
    while (node_to_swap_out->offset + remaining + 1 >= COLUMN_SIZE_DUMMY) {
        column[COLUMN_SIZE_DUMMY].m_key = column[node_to_swap_out->offset+1].m_key ;
        column[node_to_swap_out->offset+1].m_key = updates.data[posH];
        posH--;
        COLUMN_SIZE_DUMMY++;
        remaining--;
        node_to_swap_out->offset++;
    }
//    while (node_to_swap_out == lastPiece && node_to_swap_out->offset + remaining + 1 >= COLUMN_SIZE) {
//        column[COLUMN_SIZE].m_key = column[node_to_swap_out->offset+1].m_key ;
//        column[node_to_swap_out->offset+1].m_key = updates.data[posH];
//        posH--;
//        COLUMN_SIZE++;
//        remaining--;
//        node_to_swap_out->offset++;
//    }
//    verify_tree(column, T);

//    IndexEntry stored_entries[remaining];
    IndexEntry *stored_entries = (IndexEntry *) malloc(remaining * 2 * sizeof(int64_t));

    for (size_t i = 0; i < remaining; i++) {
        //assert(column[node_to_swap_out->offset + 1 + i] > high);
        stored_entries[i] = column[node_to_swap_out->offset + 1 + i];
    }

    auto next = node_to_swap_out->offset;
    // update all nodes past the one we just had with the new updated offsets
    size_t min_offset = node_to_swap_out->offset;
    size_t max_offset = min_offset + remaining;
    auto allNodes = GetNodesInOrder(T);
    for (size_t i = 0; i < allNodes.size(); i++) {
        auto &node = allNodes[i];
        if (node->offset >= min_offset && node->offset <= max_offset) {
            // this node has been getting swapped out
            // we have to increase the offset
            node->offset = max_offset;
        }
    }
    merge(column, capacity, T, updates, posL, posH, next);

    // now swap from the stored_entries into the updates
    for (auto i = 0; i < remaining; i++) {
        updates.data[posL + i] = stored_entries[i].m_key;
    }
    free(stored_entries);
}

void merge(IndexEntry *&column, size_t &capacity, AvlTree T, Column &updates, int64_t posL, int64_t posH, int64_t _next) {
    int64_t remaining = posH - posL + 1;
    int64_t ins = posH;
    int64_t next = _next < 0 ? COLUMN_SIZE_DUMMY - 1 : _next;
    int64_t prevPos = next;
    int64_t write = next + 1;
    int64_t cur = write + remaining - 1;
    int64_t w = 0;
    int64_t copy = 0;
    AvlTree firstPiece = FindFirstPiece(T);
    AvlTree node = NULL;

    if (_next < 0) {
        COLUMN_SIZE_DUMMY = COLUMN_SIZE_DUMMY + remaining;
        while (COLUMN_SIZE_DUMMY >= capacity) {
            capacity *= 2;
            column = (IndexEntry *) realloc(column, capacity * 2 * sizeof(int64_t));
        }
    }
    while (remaining > 0) {
        if (firstPiece->Element >= column[next].m_key) {
            break;
        }

        node = FindNeighborsGTFinal(column[next].m_key, (AvlTree) T);
        write = next + 1;
        cur = write + remaining - 1;
        while (remaining > 0 && (updates.data[ins] >= node->Element)) {
            column[cur].m_key = updates.data[ins];
            cur--;
            ins--;
            remaining--;
        }
        if (remaining == 0)
            break;

        next = node->offset;
        assert(prevPos >= node->offset);
        int64_t tuples = prevPos - node->offset;


        cur = next + 1;

        if (tuples > remaining) {
            w = write;
            copy = remaining;
        } else {
            w = remaining - tuples + write;
            copy = tuples;
        }
        for (size_t i = 0; i < copy; i++) {
            exchange(column, cur, w);
            cur++;
            w++;
        }
        prevPos = node->offset;
        node->offset += remaining;
    }
    if (remaining > 0) {
        w = posL;
        write = next + 1;
        cur = write + remaining - 1;
        for (size_t i = 0; i < remaining; i++) {
            column[cur].m_key = updates.data[w];
            cur--;
            w++;
        }
    }
    // verify the tree
//    verify_tree(column,T);
}
