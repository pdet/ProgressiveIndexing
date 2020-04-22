#include "progressive/progressive_quicksort.hpp"
#include "sort/sort.hpp"
#include <chrono>
#include <util/binary_search.hpp>
#include <iostream>

using namespace std;
using namespace chrono;

unique_ptr<RefinementScan> ProgressiveQuicksort::find_offsets(int64_t low, int64_t high) {
    auto offsets = make_unique<RefinementScan>();
    //! Find left side
    auto leftNode = tree->FindNodeGTE(low);
    //! Find Right side
    auto rightNode = tree->FindNodeLT(high);
    if (leftNode) {
        if (leftNode->sorted) {
            auto pieceStart = tree->pieceStart(leftNode);
            auto pieceEnd = tree->pieceEnd(leftNode);
            offsets->offsetLeft = binary_search_gte(column->data, low, pieceStart->offset, pieceEnd->offset);
            offsets->checkLeft = false;
        } else {
            auto pieceEnd = tree->pieceEnd(leftNode);
            offsets->offsetLeft = leftNode->current_start;
            offsets->offsetLeftMiddle = pieceEnd->offset;
        }
    } else {
        offsets->offsetLeft = 0;
        auto firstPiece = tree->FindMin(tree->root.get());
        offsets->offsetLeftMiddle = firstPiece->current_end;
    }
    if (rightNode) {
        if (rightNode->sorted) {
            offsets->checkRight = false;
            auto pieceStart = tree->pieceStart(rightNode);
            auto pieceEnd = tree->pieceEnd(rightNode);
            offsets->offsetRight = binary_search_lte(column->data, high, pieceStart->offset, pieceEnd->offset);
        } else {
            auto pieceStart = tree->pieceStart(rightNode);
            offsets->offsetRightMiddle = pieceStart->offset;
            offsets->offsetRight = rightNode->current_end;
        }
    } else {
        offsets->offsetRight = column->size - 1;
        auto lastPiece = tree->FindMax(tree->root.get());
        offsets->offsetRightMiddle = lastPiece->current_start;
    }
    return move(offsets);
}

ResultStruct ProgressiveQuicksort::refinement_scan(int64_t low, int64_t high) {
    ResultStruct result;
    auto offsets = find_offsets(low, high);
    //! Both pieces are sorted
    if (!offsets->checkLeft && !offsets->checkRight) {
        for (size_t i = offsets->offsetLeft; i < offsets->offsetRight; i++) {
            result.push_back(column->data[i]);
        }
    } else if (!offsets->checkLeft) { //! Only Left is Sorted
        for (size_t i = offsets->offsetLeft; i < offsets->offsetRightMiddle; i++) {
            result.push_back(column->data[i]);
        }
        //! We check the values of the right node
        for (size_t i = offsets->offsetRightMiddle; i <= offsets->offsetRight; i++) {
            int match = low <= column->data[i].m_key && column->data[i] < high;
            result.maybe_push_back(column->data[i], match);
        }
    } else if (!offsets->checkRight) { //! Only right is sorted
        for (size_t i = offsets->offsetLeft; i <= offsets->offsetLeftMiddle; i++) {
            int match = low <= column->data[i].m_key && column->data[i] < high;
            result.maybe_push_back(column->data[i], match);
        }
        for (size_t i = offsets->offsetLeftMiddle + 1; i < offsets->offsetRight; i++) {
            result.push_back(column->data[i]);
        }
    } else { //! No sorted nodes
        //! check if there are no middle pieces
        if (offsets->offsetLeft == offsets->offsetRightMiddle || offsets->offsetLeftMiddle == offsets->offsetRight) {
            for (size_t i = offsets->offsetLeft; i <= offsets->offsetLeftMiddle; i++) {
                int match = low <= column->data[i].m_key && column->data[i] < high;
                result.maybe_push_back(column->data[i], match);
            }
        } else { //! We have middle pieces
            //! We only have one middle piece that has not finished pivoting
            if (offsets->offsetLeftMiddle >= offsets->offsetRightMiddle) {
                //! We have to match everything
                for (size_t i = offsets->offsetLeft; i <= offsets->offsetRight; i++) {
                    int match = low <= column->data[i].m_key && column->data[i] < high;
                    result.maybe_push_back(column->data[i], match);
                }
            } else {
                for (size_t i = offsets->offsetLeft; i <= offsets->offsetLeftMiddle; i++) {
                    int match = low <= column->data[i].m_key && column->data[i] < high;
                    result.maybe_push_back(column->data[i], match);
                }
                //! No need to match middle pieces
                for (size_t i = offsets->offsetLeftMiddle + 1; i < offsets->offsetRightMiddle; i++) {
                    result.push_back(column->data[i]);
                }
                for (size_t i = offsets->offsetRightMiddle; i <= offsets->offsetRight; i++) {
                    int match = low <= column->data[i].m_key && column->data[i] < high;
                    result.maybe_push_back(column->data[i], match);
                }
            }
        }
    }

    return result;
}
QSAVLNode* ProgressiveQuicksort::node_refinement(QSAVLNode* node, ssize_t& remaining_swaps) {
    if (!node || node->sorted) {
        return node;
    }
    //! Check if node has children
    if (node->noChildren()) { //! No children
        auto pieceStart = tree->pieceStart(node);
        auto pieceEnd = tree->pieceEnd(node);
        if ((pieceEnd->offset - pieceStart->offset) <= 1024) { //! small enough to sort?
            //! node is very small, just sort it
            if (remaining_swaps > (pieceEnd->offset - pieceStart->offset) * 15) {
                hybrid_radixsort_insert(column->data + pieceStart->offset, pieceEnd->offset - pieceStart->offset + 1);
                node->sorted = true;
                //! Check if there are nodes we should merge
                auto parent = tree->findParent(node);
                remaining_swaps -= (pieceEnd->offset - pieceStart->offset) * 15; //! log2(8192)
                node = tree->mergeChildren(parent);
            }
            return node;
        }
        while (node->current_start < node->current_end && remaining_swaps > 0) {  //! Continue pivoting
            //! TODO: we could scan while we pivot here
            auto start = column->data[node->current_start];
            auto end = column->data[node->current_end];

            int start_has_to_swap = start >= node->pivot;
            int end_has_to_swap = end < node->pivot;
            int has_to_swap = start_has_to_swap * end_has_to_swap;

            column->data[node->current_start].m_key = !has_to_swap * start.m_key + has_to_swap * end.m_key;
            column->data[node->current_end].m_key = !has_to_swap * end.m_key + has_to_swap * start.m_key;
            column->data[node->current_start].m_rowId = !has_to_swap * start.m_rowId + has_to_swap * end.m_rowId;
            column->data[node->current_end].m_rowId = !has_to_swap * end.m_rowId + has_to_swap * start.m_rowId;

            node->current_start += !start_has_to_swap + has_to_swap;
            node->current_end -= !end_has_to_swap + has_to_swap;

            remaining_swaps--;
        }
        if (node->current_start >= node->current_end && !node->sorted) { //! We've finished  pivoting
            //! Time to procreate
            //! Left Node
            size_t current_start = pieceStart->offset;
            size_t current_end = node->current_end;
            int64_t pivot = (pieceStart->value + node->pivot) / 2;
            assert(pivot < node->pivot);
            node->setLeft(make_unique<QSAVLNode>(pivot, current_start, current_end));

            //! Right node
            current_start = current_end;
            current_end = pieceEnd->offset;
            pivot = (pieceEnd->value + node->pivot) / 2;
            assert(pivot > node->pivot);
            node->setRight(make_unique<QSAVLNode>(pivot, current_start, current_end));
        }
    } else {
        //! Node has children, visit them
        node_refinement(node->left.get(), remaining_swaps);
        node_refinement(node->right.get(), remaining_swaps);
    }
    return node;
}
void ProgressiveQuicksort::refine(ssize_t& remaining_swaps, int64_t low, int64_t high) {
    //! Get Boundaries for query
    auto lowNode = tree->FindNodeGTE(low);
    //! if lowNode is null we get first node
    if (!lowNode) {
        lowNode = tree->FindMin(tree->root.get());
    }
    //! Prioritize a bit of cracking on the boundary nodes
    node_refinement(lowNode, remaining_swaps);
    auto highNode = tree->FindNodeLT(high);
    if (!highNode) {
        highNode = lowNode;
    }
    node_refinement(highNode, remaining_swaps);

    //! If we still have budget after finishing the priority cracking we keep on cracking
    auto node = tree->FindMin(tree->root.get());
    while (remaining_swaps > 1024 * 15 && !tree->root->sorted) {
        if (!node) {
            node = tree->FindMin(tree->root.get());
        }
        assert(node);
        node = node_refinement(node, remaining_swaps);
        node = tree->inOrderSucessor(node);
    }
    assert(remaining_swaps <= 1024 * 15 || tree->root->sorted);
}

ResultStruct ProgressiveQuicksort::create(vector<int64_t>& originalColumn, ssize_t& remaining_swaps, int64_t low, int64_t high) {
    ResultStruct results;
    auto indexColumn = column->data;
    auto root = (QSAVLNode*)tree->root.get();
    size_t next_index;
    //! for the initial run, we write the indices instead of swapping them
    //! because the current array has not been initialized yet
    //! first look through the part we have already pivoted
    //! for data that matches the points
        if (low <= root->pivot) {
            for (size_t i = 0; i < root->current_start; i++) {
                int matching = indexColumn[i] >= low && indexColumn[i] < high;
                results.maybe_push_back(indexColumn[i], matching);
            }
        }
        if (high >= root->pivot) {
            for (size_t i = root->current_end + 1; i < originalColumn.size(); i++) {
                int matching = indexColumn[i] >= low && indexColumn[i] < high;
                results.maybe_push_back(indexColumn[i], matching);
            }
        }
        //! now we start filling the index with at most remaining_swap entries
        next_index = min(current_position + remaining_swaps, originalColumn.size());
        remaining_swaps -= next_index - current_position;
        for (size_t i = current_position; i < next_index; i++) {
            int matching = originalColumn[i] >= low && originalColumn[i] < high;
            results.maybe_push_back(originalColumn[i], matching);

            int bigger_pivot = originalColumn[i] >= root->pivot;
            int smaller_pivot = 1 - bigger_pivot;

            indexColumn[root->current_start].m_key = originalColumn[i];
            indexColumn[root->current_start].m_rowId = i;
            indexColumn[root->current_end].m_key = originalColumn[i];
            indexColumn[root->current_end].m_rowId = i;

            root->current_start += smaller_pivot;
            root->current_end -= bigger_pivot;
        }
        current_position = next_index;

    //! Check if we are finished with the initial run
    if (next_index == originalColumn.size()) {
//        cout << "created" << endl;
        assert(root->current_start >= root->current_end);
        //! construct the left and right side of the root node
        int64_t pivot = column->size / 4;
        size_t current_start = 0;
        size_t current_end = root->current_end;

        root->setLeft(make_unique<QSAVLNode>(pivot, current_start, current_end));

        //! Right node
        pivot = (column->size / 4) * 3;
        current_start = current_end + 1;
        current_end = column->size - 1;
        root->setRight(make_unique<QSAVLNode>(pivot, current_start, current_end));
    } else {
        //! we have done all the swapping for this run
        //! now we query the remainder of the data
        for (size_t i = current_position; i < originalColumn.size(); i++) {
            int matching = originalColumn[i] >= low && originalColumn[i] < high;
            results.maybe_push_back(originalColumn[i], matching);
        }
    }
    return results;
}

double ProgressiveQuicksort::get_cost_for_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, double estimatedDelta) {
    //! If converged
    if (fullIndex.get()) {
        return 0;
    }
    double page_count = (originalColumn.size() / constants->ELEMENTS_PER_PAGE) + (originalColumn.size() % ((int)constants->ELEMENTS_PER_PAGE) != 0 ? 1 : 0);
    double scan_speed = constants->READ_ONE_PAGE_SEQ_MS * page_count;
    //! First Query
    if (!tree) {
        double pivot_speed = constants->WRITE_ONE_PAGE_SEQ_MS * page_count;
        return ((1 - estimatedDelta) * scan_speed + estimatedDelta * pivot_speed) / 10.0;
    }

    auto root = tree->root.get();
    if (root && root->noChildren() && !root->sorted) {
        //! figure out unindexed fraction
        double rho = (double)current_position / (double)originalColumn.size();
        //! figure out fraction of index to scan, either low or high or both
        double alpha = 0;
        if (current_position > 0) {
            if (low < root->pivot) {
                alpha += (double)root->current_start / (double)originalColumn.size();
            }
            if (high >= root->pivot) {
                alpha += (double)(originalColumn.size() - root->current_end) / (double)originalColumn.size();
            }
        }
        double pivot_speed = constants->WRITE_ONE_PAGE_SEQ_MS * page_count;
        return ((1 - rho + alpha - estimatedDelta) * scan_speed + estimatedDelta * pivot_speed) / 1000.0;
    } else {
        size_t height = tree->getHeight(root);
        double lookup_speed = height * constants->RANDOM_ACCESS_PAGE_MS;
        double refine_speed = constants->WRITE_ONE_PAGE_SEQ_MS * page_count;

        //! figure out alpha
        auto offsets = find_offsets(low, high);

        double alpha = (double)(offsets->offsetRight - offsets->offsetLeft) / (double)originalColumn.size();
        assert(alpha <= 1);
        return (lookup_speed + alpha * scan_speed + estimatedDelta * refine_speed) / 1000.0;
    }
}

double ProgressiveQuicksort::get_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high) {
    size_t ITERATIONS = 20;
    double estimated_delta = 0.5;
    double offset = estimated_delta / 2;
    double estimated_time = 0;

    for (size_t j = 0; j < ITERATIONS; j++) {
        estimated_time = get_cost_for_delta(originalColumn, low, high, estimated_delta);
        if (estimated_time > time_threshold) {
            estimated_delta -= offset;
        } else {
            estimated_delta += offset;
        }
        offset /= 2;
    }
    return estimated_delta;
}

int64_t ProgressiveQuicksort::run_query(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, std::vector<double>& time, size_t query_it) {
    if (interactivity_threshold != 0) {
        delta = get_delta(originalColumn, low, high);
    }
    //! Amount of work we are allowed to do
    auto remaining_swaps = (ssize_t)(originalColumn.size() * delta);
    //! Initialize Index (e.g., reserve sizes and all)
    if (!column.get()) {
        size_t column_size = originalColumn.size();
        //! Reserve column
        column = make_unique<IdxCol>(column_size);
        //! We select the element in the middle of the column as the pivot
        int64_t pivot = column_size / 2;
        initializeRoot(pivot, column_size);
    }
    ResultStruct result;
    //! Index Creation/Refinement or Consolidation

    auto startTimer = chrono::system_clock::now();
    if (tree->root.get() && tree->noChildren() && !tree->root->sorted) { //! If the node has no children we are still in the creation phase
        //! Creation Phase
        //! TODO: We might still have swaps left to do after the  creation phase Need to implement a smooth transition
        result = create(originalColumn, remaining_swaps, low, high);
//        cout << query_it << "  creating"<< endl;
    } else if (tree->root.get() && !tree->noChildren()) {
        //! Refinement Phase
        //! TODO: We might still have swaps left to do after the refinement phase Need to implement a smooth transition
        refine(remaining_swaps, low, high);
        result = refinement_scan(low, high);
//        if (true) {
//            column->check_column();
//            int64_t true_result = column->full_scan( low, high);
//            assert(true_result == result.sum);
//        }
    } else {
        //! Consolidation Phase
        if (!fullIndex.get()) {
//            cout << "converged" << endl;
            consolidate(remaining_swaps);
        }
        result = consolidate_scan(low, high);
    }
    auto endTimer = system_clock::now();
   time[query_it] += duration<double>(endTimer - startTimer).count();

//    cout <<  time[query_it] << "\n";

    return result.sum;
}
