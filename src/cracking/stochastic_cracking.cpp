#include "../include/cracking/stochastic_cracking.h"

#include <assert.h>

int64_t randomNumber(int64_t max) {
	return 1 + (int64_t)(max * (double)rand() / (RAND_MAX + 1.0));
}

IntPair crackInTwoMDD1RBranched(IndexEntry *&c, int64_t posL, int64_t posH, int64_t low, int64_t high,
                                IndexEntry *&view, int64_t &view_size) {

	int64_t L = posL;
	int64_t R = posH;
	int64_t a = low;
	int64_t b = high;

	view = (IndexEntry *)malloc((R - L + 1) * sizeof(IndexEntry)); //! initialize view to the maximum possible size
	int64_t size = 0;

	int64_t x = c[L + randomNumber(R - L + 1) - 1].m_key;

	while (L <= R) {
		while (L <= R && c[L] < x) {
			if (c[L] >= a && c[L] < b)
				view[size++] = c[L];
			L = L + 1;
		}
		while (L <= R && c[R] >= x) {
			if (c[R] >= a && c[R] < b)
				view[size++] = c[R];
			R = R - 1;
		}
		if (L < R)
			exchange(c, L, R);
	}

	view_size = size;

	//! add crack on X at position L
	IntPair p = (IntPair)malloc(sizeof(struct int_pair));
	p->first = x;
	p->second = L - 1;
	return p;
}

IntPair crackInTwoMDD1RPredicated(IndexEntry *&c, int64_t posL, int64_t posH, int64_t low, int64_t high,
                                  IndexEntry *&view, int64_t &view_size) {
	view =
	    (IndexEntry *)malloc((posH - posL + 1) * sizeof(IndexEntry)); //! initialize view to the maximum possible size
	int64_t size = 0;

	int64_t pivot = c[posL + randomNumber(posH - posL + 1) - 1].m_key;

	IndexEntry *begin = &c[posL];
	IndexEntry *end = &c[posH + 1];
	/* Corner case handling for odd number of elements. */
	if ((end - begin) & 0x1) {
		int matching = end[-1] >= low && end[-1] < high;
		view[size] = end[-1];
		size += matching;
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
		int matching = first >= low && first < high;
		view[size] = first;
		size += matching;
		matching = second >= low && second < high;
		view[size] = second;
		size += matching;
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
	while (posL > 0 && c[posL] >= pivot) {
		posL--;
	}
	view_size = size;

	//! add crack on X at position L
	IntPair p = (IntPair)malloc(sizeof(struct int_pair));
	p->first = pivot;
	p->second = posL;
	return p;
}

AvlTree stochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey,
                           QueryOutput *qo, CrackEngineType engineType) {

	IntPair p1, p2;

	p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
	p2 = FindNeighborsLT(highKey, T, dataSize - 1);

	IntPair pivot_pair = NULL;

	if (p1->first == p2->first && p1->second == p2->second) {
		switch (engineType) {
		case CrackEngineType ::Branched: {
			pivot_pair = crackInTwoMDD1RBranched(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);
		} break;
		case CrackEngineType::Predicated: {
			pivot_pair =
			    crackInTwoMDD1RPredicated(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);
		} break;
		case CrackEngineType::Rewired:
			break;
		}
		lowKey = pivot_pair->first;
		highKey = pivot_pair->first;
		pivot_pair->first = pivot_pair->second;
	} else {

		pivot_pair = (IntPair)malloc(sizeof(struct int_pair));
		IntPair pivot_pair1;
		switch (engineType) {
		case CrackEngineType ::Branched: {
			pivot_pair1 = crackInTwoMDD1RBranched(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);

		} break;
		case CrackEngineType::Predicated: {
			pivot_pair1 =
			    crackInTwoMDD1RPredicated(c, p1->first, p1->second, lowKey, highKey, qo->view1, qo->view_size1);
		} break;
		case CrackEngineType::Rewired:
			break;
		}
		qo->middlePart = &c[p1->second + 1];
		int size2 = p2->first - p1->second - 1;
		qo->middlePart_size = size2;
		IntPair pivot_pair2;
		switch (engineType) {
		case CrackEngineType ::Branched: {
			pivot_pair2 = crackInTwoMDD1RBranched(c, p2->first, p2->second, lowKey, highKey, qo->view2, qo->view_size2);

		} break;
		case CrackEngineType::Predicated: {
			pivot_pair2 =
			    crackInTwoMDD1RPredicated(c, p2->first, p2->second, lowKey, highKey, qo->view2, qo->view_size2);
		} break;
		case CrackEngineType::Rewired:
			break;
		}
		pivot_pair->first = pivot_pair1->second;
		lowKey = pivot_pair1->first;
		pivot_pair->second = pivot_pair2->second;
		highKey = pivot_pair2->first;

		free(pivot_pair1);
		free(pivot_pair2);
	}

	T = Insert(pivot_pair->first, lowKey, T);
	T = Insert(pivot_pair->second, highKey, T);

	free(p1);
	free(p2);
	if (pivot_pair) {
		free(pivot_pair);
		pivot_pair = NULL;
	}

	return T;
}
