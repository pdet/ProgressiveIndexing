#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include "../include/sort/sort.hpp"
#include "index/index.hpp"

using namespace std;
using namespace chrono;

int main(int argc, char** argv) {
    time_point<system_clock> startTimer, endTimer;

    int size = 1024;
    double sortTime = 0.0;
    double  swapTime = 0.0;
    int64_t* data = (int64_t*)malloc(sizeof(int64_t) * size);
    IdxColEntry* column = (IdxColEntry*)malloc(sizeof(IdxColEntry) * size);
    for (size_t i = 0; i < size; i++) {
        data[i] = i;
    }
    shuffle(data, data + size, default_random_engine(1));
    for (size_t i = 0; i < 10; i ++){
        for (size_t i = 0; i < size; i++) {
            column[i].m_key = i;
            column[i].m_rowId = data[i];
        }

        //! Cost to sort
        startTimer = system_clock::now();
        hybrid_radixsort_insert(column,size);
        endTimer = system_clock::now();
        sortTime += duration<double>(endTimer - startTimer).count();
    }
    sortTime /= 10;
    cout << "Sort 1024 elements: " << sortTime << endl;
    size = pow(10, 8);
    auto data_2 = (int64_t*)malloc(sizeof(int64_t) * size);
    IdxColEntry* column_2 = (IdxColEntry*)malloc(sizeof(IdxColEntry) * size);
    for (size_t i = 0; i < size; i++) {
        data_2[i] = i;
    }
    shuffle(data_2, data_2 + size, default_random_engine(1));
    for (size_t i = 0; i < 10; i ++){
        for (size_t i = 0; i < size; i++) {
            column_2[i].m_key = i;
            column_2[i].m_rowId = data_2[i];
        }
        int64_t pivot = size/2;
        int64_t swaps = 0;
        //! Figure out cost of one swap
        startTimer = system_clock::now();
        auto current_start = 0;
        auto current_end = size-1;
        while (current_end > current_start){
            auto start = column_2[0];
            auto end = column_2[size-1];

            int start_has_to_swap = start >= pivot;
            int end_has_to_swap = end < pivot;
            int has_to_swap = start_has_to_swap * end_has_to_swap;

            column_2[current_start].m_key = !has_to_swap * start.m_key + has_to_swap * end.m_key;
            column_2[current_end].m_key = !has_to_swap * end.m_key + has_to_swap * start.m_key;
            column_2[current_start].m_rowId = !has_to_swap * start.m_rowId + has_to_swap * end.m_rowId;
            column_2[current_end].m_rowId = !has_to_swap * end.m_rowId + has_to_swap * start.m_rowId;

            current_start += !start_has_to_swap + has_to_swap;
            current_end -= !end_has_to_swap + has_to_swap;

            swaps++;
        }

        endTimer = system_clock::now();
        swapTime += duration<double>(endTimer - startTimer).count()/swaps;
    }
    swapTime /=10;
    cout << "Cost per swap: " << swapTime << endl;
    cout << "Total swaps in sorting: " << sortTime/swapTime << endl;
    cout << "Swap Rate: " << sortTime/(swapTime*1024) << endl;

}
