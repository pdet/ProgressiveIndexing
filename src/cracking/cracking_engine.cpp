#include "../include/cracking/cracking_engine.h"

#include <assert.h>
#include <sys/mman.h>

int numSwapps = 0;

CrackEngineType adaptive_cracking_engine(IndexEntry *c, AvlTree T, int64_t low_query, int64_t high_query,
                                         int64_t data_size) {
	IntPair p1 = FindNeighborsGTE(low_query, (AvlTree)T, data_size - 1);
	IntPair p2 = FindNeighborsLT(high_query, (AvlTree)T, data_size - 1);
	int offset1 = p1->first;
	int offset2 = p2->second;
	int selectivity = ((offset2 - offset1) * 100) / data_size;
	if (selectivity < 5 || selectivity > 95)
		return CrackEngineType::Branched;
	else if (sizeof((c[0].m_key)) == 8)
		return CrackEngineType::Predicated;
	else
		return CrackEngineType::Rewired;
}

void exchange(IndexEntry *&c, int64_t x1, int64_t x2) {
	IndexEntry tmp = *(c + x1);
	*(c + x1) = *(c + x2);
	*(c + x2) = tmp;
	numSwapps++;
}

int64_t crackInTwoBranched(IndexEntry *&c, int64_t posL, int64_t posH, int64_t med) {
	int64_t x1 = posL, x2 = posH;
	while (x1 <= x2) {
		if (c[x1] < med)
			x1++;
		else {
			while (x2 >= x1 && (c[x2] >= med))
				x2--;
			if (x1 < x2) {
				exchange(c, x1, x2);
				x1++;
				x2--;
			}
		}
	}
	if (x1 < x2)
		printf("Not all elements were inspected!");
	x1--;
	if (x1 < 0)
		x1 = 0;
	return x1;
}

void checkColumn(IndexEntry *&c, int64_t pos, int64_t pivot, int64_t column_size) {
	for (size_t i = 0; i <= column_size; i++) {
		if (i <= pos) {
			if (c[i] >= pivot) {
				fprintf(stderr, "Error 1\n");
				assert(0);
			}
		} else if (c[i] < pivot) {
			fprintf(stderr, "Error 2\n");
			assert(0);
		}
	}
}

int64_t crackInTwoPredicated(IndexEntry *&c, int64_t posL, int64_t posH, int64_t pivot) {
	IndexEntry *begin = &c[posL];
	IndexEntry *end = &c[posH + 1];
	/* Corner case handling for odd number of elements. */
	if ((end - begin) & 0x1) {
		if (end[-1] >= pivot)
			--end;
		else {
			using std::swap;
			swap(end[-1], *begin);
			++begin;
			++posL;
		}
	}
	assert(not(end - begin & 0x1) && "not a multiple of 2");

	auto first = *begin;
	auto second = end[-1];

	while (begin < end) {
		{
			*begin = end[-1] = first;
			auto left = begin[1];
			auto right = end[-2];
			const ptrdiff_t advance_lower = first < pivot;
			begin += advance_lower;
			posL += advance_lower;
			end += ptrdiff_t(-1) + advance_lower;
			first = advance_lower ? left : right;
		}
		{
			*begin = end[-1] = second;
			auto left = begin[1];
			auto right = end[-2];
			const ptrdiff_t advance_lower = second < pivot;
			begin += advance_lower;
			posL += advance_lower;
			end += ptrdiff_t(-1) + advance_lower;
			second = advance_lower ? left : right;
		}
	}
	while (posL > 0 && c[posL] >= pivot)
		posL--;
	return posL;
}
