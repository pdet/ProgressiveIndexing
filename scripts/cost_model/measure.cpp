#include <chrono>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <climits>
#include <cmath>
#include <iostream>
#include <algorithm>

#define PAGESIZE 4096
#define ELEMENTS_PER_PAGE (PAGESIZE / sizeof(int64_t))

#define PAGES_TO_WRITE 100000

#define ELEMENT_COUNT (PAGES_TO_WRITE * ELEMENTS_PER_PAGE)

int main() {
	int64_t *base_column = new int64_t[PAGES_TO_WRITE * ELEMENTS_PER_PAGE];
	int64_t *values = new int64_t[PAGES_TO_WRITE * ELEMENTS_PER_PAGE];
	size_t *index = new size_t[ELEMENT_COUNT];
	ssize_t remaining_swaps;
	
	struct {
		size_t current_start;
		size_t current_end;
		size_t pivot = 5000;
	} node;
	node.current_start = 0;
	node.current_end = ELEMENT_COUNT-1;
	int64_t low = 1000;
	int64_t high = 20000;
	int64_t sum = 0;

    for(size_t i = 0; i < ELEMENT_COUNT; i++) {
    	base_column[i] = rand() % 50000;
    }

	// initial write
    auto start = std::chrono::system_clock::now();
    for(size_t i = 0; i < ELEMENT_COUNT; i++) {
        int matching = base_column[i] >= low && base_column[i] <= high;
        sum += matching * base_column[i];

        int bigger_pivot = base_column[i] >= node.pivot;
        int smaller_pivot = 1 - bigger_pivot;

        values[node.current_start] = base_column[i];
        values[node.current_end] = base_column[i];
        index[node.current_start] = base_column[i];
        index[node.current_end] = base_column[i];

        node.current_start += smaller_pivot;
        node.current_end -= bigger_pivot;
    }
	auto end = std::chrono::system_clock::now();

	double s = std::chrono::duration<double>(end - start).count();
    std::cout << "#define WRITE_ONE_PAGE_SEQ_MS " << (s*1000)/PAGES_TO_WRITE << "\n";
	// reading
	start = std::chrono::system_clock::now();
    for(size_t i = 0; i < ELEMENT_COUNT; i++) {
	    int matching = values[i] >= low &&
	                   values[i] <= high;
	    sum += values[i] * matching;
    }
	end = std::chrono::system_clock::now();
    // Avoiding -O3 Optimization
    if (sum != 0)
        fprintf(stderr, " " );
	s = std::chrono::duration<double>(end - start).count();
    std::cout << "#define READ_ONE_PAGE_SEQ_MS " << (s*1000)/PAGES_TO_WRITE << "\n";

    // reading
    start = std::chrono::system_clock::now();
    for(size_t i = 0; i < ELEMENT_COUNT; i++) {
        sum += values[i];
    }
    end = std::chrono::system_clock::now();
    // Avoiding -O3 Optimization
    if (sum != 0)
        fprintf(stderr, " " );
    s = std::chrono::duration<double>(end - start).count();
    std::cout << "#define READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS " << (s*1000)/PAGES_TO_WRITE << "\n";


	std::vector<int> random_lookups;
	for(size_t i = 0; i < PAGES_TO_WRITE; i++) {
		random_lookups.push_back(rand() % PAGES_TO_WRITE);
	}

	// random page access
	start = std::chrono::system_clock::now();
	for(size_t i = 0; i < PAGES_TO_WRITE; i++) {
		sum += values[random_lookups[i]];
	}
	end = std::chrono::system_clock::now();
    // Avoiding -O3 Optimization
    if (sum != 0)
        fprintf(stderr, " " );
	s = std::chrono::duration<double>(end - start).count();
    std::cout << "#define RANDOM_ACCESS_PAGE_MS " << (s*1000)/PAGES_TO_WRITE << "\n";


	for(size_t i = 0; i < ELEMENT_COUNT; i++) {
		values[i] = rand() % 10000;
		index[i] = i;
	}
	
	node.current_start = 0;
	node.current_end = ELEMENT_COUNT-1;

	// swapping
	remaining_swaps = ELEMENT_COUNT;
	start = std::chrono::system_clock::now();
	while (node.current_start < node.current_end &&
	           remaining_swaps > 0) {
        int64_t start = values[node.current_start];
        int64_t end = values[node.current_end];
        size_t start_pointer = index[node.current_start];
        size_t end_pointer = index[node.current_end];

        int start_has_to_swap = start >= node.pivot;
        int end_has_to_swap = end < node.pivot;
        int has_to_swap = start_has_to_swap * end_has_to_swap;

        values[node.current_start] =
            !has_to_swap * start + has_to_swap * end;
        values[node.current_end] =
            !has_to_swap * end + has_to_swap * start;
        index[node.current_start]
            = !has_to_swap * start_pointer +
               has_to_swap * end_pointer;
        index[node.current_end]
            = !has_to_swap * end_pointer +
               has_to_swap * start_pointer;

        node.current_start += !start_has_to_swap + has_to_swap;
        node.current_end -= !end_has_to_swap + has_to_swap;
        remaining_swaps--;
    }
	end = std::chrono::system_clock::now();

	s = std::chrono::duration<double>(end - start).count();
    std::cout << "#define SWAP_COST_PAGE_MS " <<  (s*1000)/PAGES_TO_WRITE << "\n";

}