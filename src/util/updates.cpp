void cracking_merge_complete(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                             vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            sort(begin(updates->data), std::end(updates->data));
            merge(crackercolumn, capacity, T, *updates, 0, updates->data.size() - 1);
            updates->data.clear();
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void cracking_merge_gradually(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                              vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        bool updates_is_sorted = false;
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        if (updates->data.size()) {
            int64_t initial_offset = 0;
            int64_t final_offset = 0;
            if (!updates_is_sorted) {
                sort(begin(updates->data), std::end(updates->data));
                updates_is_sorted = true;
            }
            initial_offset = binary_search_gte(&updates->data[0], rangeQueries.leftpredicate[i], 0,
                                               updates->data.size() - 1);
            if (initial_offset > 0)
                initial_offset--;
            final_offset = binary_search_gte(&updates->data[0], rangeQueries.rightpredicate[i], 0,
                                             updates->data.size() - 1);
            if (final_offset < updates->data.size() - 1)
                final_offset++;
            merge(crackercolumn, capacity, T, *updates, initial_offset, final_offset);
            updates->data.erase(updates->data.begin() + initial_offset, updates->data.begin() + final_offset + 1);

        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void cracking_merge_ripple(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                           vector<double> &times) {
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    chrono::time_point<chrono::system_clock> start, end;
    chrono::duration<double> elapsed_seconds;
    Column *updates = new Column();
    COLUMN_SIZE_DUMMY = COLUMN_SIZE;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;

    start = chrono::system_clock::now();
    size_t capacity = COLUMN_SIZE_DUMMY;
    IndexEntry *crackercolumn = (IndexEntry *) malloc(COLUMN_SIZE_DUMMY * 2 * sizeof(int64_t));
    //Creating Cracker Column
    for (size_t i = 0; i < COLUMN_SIZE_DUMMY; i++) {
        crackercolumn[i].m_key = column.data[i];
        crackercolumn[i].m_rowId = i;
    }
    //Initialitizing Cracker Index
    AvlTree T = NULL;
    end = chrono::system_clock::now();
    times[0] += chrono::duration<double>(end - start).count();
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        start = chrono::system_clock::now();
        // Time to fill append list
        bool updates_is_sorted = false;
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        if (updates->data.size()) {
            int64_t initial_offset = 0;
            int64_t final_offset = 0;
            sort(begin(updates->data), std::end(updates->data));
            initial_offset = binary_search_gte(&updates->data[0], rangeQueries.leftpredicate[i], 0,
                                               updates->data.size() - 1);
            final_offset = binary_search_gte(&updates->data[0], rangeQueries.rightpredicate[i], 0,
                                             updates->data.size() - 1);
            if (initial_offset < final_offset) {
                while (final_offset < updates->data.size() &&
                       updates->data[final_offset] <= rangeQueries.rightpredicate[i]) {
                    final_offset++;
                }
                merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset - 1,
                             rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);
            } else if (final_offset == updates->data.size() - 1 && initial_offset == updates->data.size() - 1
                       && updates->data[final_offset] >= rangeQueries.leftpredicate[i] &&
                       updates->data[final_offset] <= rangeQueries.rightpredicate[i])
                merge_ripple(crackercolumn, capacity, T, *updates, initial_offset, final_offset,
                             rangeQueries.leftpredicate[i], rangeQueries.rightpredicate[i]);

        }
        T = standardCracking(crackercolumn, COLUMN_SIZE_DUMMY, T, rangeQueries.leftpredicate[i],
                             rangeQueries.rightpredicate[i] + 1);
        //Querying
        IntPair p1 = FindNeighborsGTE(rangeQueries.leftpredicate[i], (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        IntPair p2 = FindNeighborsLT(rangeQueries.rightpredicate[i] + 1, (AvlTree) T, COLUMN_SIZE_DUMMY - 1);
        int offset1 = p1->first;
        int offset2 = p2->second;
        free(p1);
        free(p2);
        int64_t sum = scanQuery(crackercolumn, offset1, offset2);
        end = chrono::system_clock::now();
        times[i] += chrono::duration<double>(end - start).count();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    sum);
        }
    }
    free(crackercolumn);
    updates->Clear();
}

void
progressive_mergesort(Column &column, RangeQuery &rangeQueries, vector<int64_t> &answers,
                      vector<double> &times) {
    chrono::time_point<chrono::system_clock> start, end;
    vector<int64_t> to_do_updates(NUM_UPDATES * FREQUENCY);
    generate_updates(to_do_updates);
    bool converged = false;
    Column *updates = new Column();
    std::vector<Column *> sort_chunks;
    Column *merge_column = nullptr;
    size_t left_column = 0, right_column = 0, merge_index = 0;
    ssize_t left_chunk, right_chunk;

    std::vector<bool> has_converged;
    size_t update_index = 0;
    size_t update_count = to_do_updates.size() / FREQUENCY;
    int unsorted_column_count = 1;
    double SORTED_COLUMN_RATIO = 16;

    for (int i = 0; i < NUM_QUERIES; i++) {
        // Time to fill append list
        start = chrono::system_clock::now();
        if (((i + 1) % FREQUENCY) == 0 && i > 0) {
            DELTA = 0.05;
            for (size_t j = 0; j < update_count && update_index + j < to_do_updates.size(); j++) {
                updates->data.push_back(to_do_updates[update_index + j]);
            }
            update_index = std::min(update_index + update_count, to_do_updates.size());
        }
        ResultStruct results;

        double original_delta = unsorted_column_count == 0 ? 0 : DELTA / unsorted_column_count;
        results = range_query_incremental_quicksort(column, rangeQueries.leftpredicate[i],
                                                    rangeQueries.rightpredicate[i], original_delta);
        if (!converged && column.converged) {
            converged = true;
            unsorted_column_count--;
        }
        for (size_t j = 0; j < sort_chunks.size(); j++) {
            auto &chunk = sort_chunks[j];
            double sort_chunk_delta = original_delta * ((double) column.data.size() / (double) chunk->data.size());
            results.merge(range_query_incremental_quicksort(*chunk, rangeQueries.leftpredicate[i],
                                                            rangeQueries.rightpredicate[i], sort_chunk_delta));
            if (!has_converged[j] && chunk->converged) {
                has_converged[j] = true;
                unsorted_column_count--;
            }
        }
        for (auto &element : updates->data) {
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            results.maybe_push_back(element, matching);
        }

        if (!merge_column && unsorted_column_count == 0 && sort_chunks.size() > 1) {
            // start a merge
            merge_column = new Column();
            left_chunk = sort_chunks.size() - 2;
            right_chunk = sort_chunks.size() - 1;
            merge_column->data.resize(sort_chunks[left_chunk]->data.size() + sort_chunks[right_chunk]->data.size());
            left_column = 0;
            right_column = 0;
            merge_index = 0;
        }
        if (merge_column) {
            ssize_t todo_merge = std::min((ssize_t) merge_column->data.size() - merge_index,
                                          (ssize_t) DELTA * column.data.size());
            for (size_t j = 0; j < todo_merge; j++) {
                if (left_column < sort_chunks[left_chunk]->data.size() &&
                    (right_column >= sort_chunks[right_chunk]->data.size() ||
                     sort_chunks[left_chunk]->data[left_column] < sort_chunks[right_chunk]->data[right_column])) {
                    merge_column->data[merge_index++] = sort_chunks[left_chunk]->data[left_column++];
                } else {
                    merge_column->data[merge_index++] = sort_chunks[right_chunk]->data[right_column++];
                }
            }
            if (merge_index == merge_column->data.size()) {
                // finish merging
                delete sort_chunks[left_chunk];
                delete sort_chunks[right_chunk];
                sort_chunks.erase(sort_chunks.begin() + left_chunk, sort_chunks.begin() + right_chunk + 1);
                sort_chunks.insert(sort_chunks.begin(), merge_column);
                merge_column = nullptr;
            }
        }

        end = chrono::system_clock::now();
        int64_t correct_answer = answers[i];
        for (size_t j = 0; j < update_index; j++) {
            auto &element = to_do_updates[j];
            int matching = element >= rangeQueries.leftpredicate[i] && element <= rangeQueries.rightpredicate[i];
            correct_answer += element * matching;
        }
        if (results.sum != correct_answer) {
            fprintf(stderr, "Incorrect Results on query %lld\n Expected : %lld    Got : %lld \n", i, correct_answer,
                    results.sum);
        }

        times[i] += chrono::duration<double>(end - start).count();
        if (updates->data.size() > column.data.size() / SORTED_COLUMN_RATIO) {
            // start a quick sort on the updates
            sort_chunks.push_back(updates);
            has_converged.push_back(false);
            updates = new Column();
            unsorted_column_count++;
        }
    }
    updates->Clear();

}