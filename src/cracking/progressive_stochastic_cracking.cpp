#include "../include/cracking/progressive_stochastic_cracking.h"

#include "../include/cracking/stochastic_cracking.h"

#include <assert.h>
#include <map>

using namespace std;

extern int64_t L2_SIZE, COLUMN_SIZE;
extern double ALLOWED_SWAPS_PERCENTAGE;

map<int64_t, pair<int64_t, pair<int64_t, int64_t>>> partial_crack;

IntPair mdd1rp_split_and_materializeBranched(IndexEntry *&c, pair<int64_t, pair<int64_t, int64_t>> &P, int64_t L,
                                             int64_t R, int64_t a, int64_t b, int64_t nswap, IndexEntry *&view,
                                             int64_t &view_size) {
	int64_t X = P.first;
	int64_t &pL = P.second.first, &pR = P.second.second;
	if (view)
		free(view);
	view = (IndexEntry *)malloc((R - L + 1) * sizeof(IndexEntry)); //! initialize view to the maximum possible size

	//! optimization based on where [a,b] relative to X
	if (b < X) {
		assert(pR <= R);
		R = pR; //! skip the right section

		//! fast scan the left
		while (L < pL) {
			if (c[L] >= a && c[L] < b)
				view[view_size++] = c[L]; //! materialize
			L++;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X) {
				if (c[L] >= a && c[L] < b)
					view[view_size++] = c[L]; //! materialize
				L++;
			}
			while (L <= R && c[R] >= X)
				R--; //! the arr[R] don't need to be checked for materialization!
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	} else if (X <= a) {
		assert(L <= pL);
		L = pL; //! skip the left section

		//! fast scan the right
		while (pR < R) {
			if (c[R] >= a && c[R] < b)
				view[view_size++] = c[R]; //! materialize
			R--;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X)
				L++; //! the arr[L] don't need to be checked for materialization!
			while (L <= R && c[R] >= X) {
				if (c[R] >= a && c[R] < b)
					view[view_size++] = c[R]; //! materialize
				R--;
			}
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	} else {
		//! fast scan left and right
		while (L < pL) {
			if (c[L] >= a && c[L] < b)
				view[view_size++] = c[L]; //! materialize
			L++;
		}
		while (pR < R) {
			if (c[R] >= a && c[R] < b)
				view[view_size++] = c[R]; //! materialize
			R--;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X) {
				if (c[L] >= a)
					view[view_size++] = c[L]; //! materialize
				L++;
			}
			while (L <= R && c[R] >= X) {
				if (c[R] < b)
					view[view_size++] = c[R]; //! materialize
				R--;
			}
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	}
	pL = L;
	pR = R;

	if (L > R) {
		//! add crack on X at position L
		IntPair p = (IntPair)malloc(sizeof(struct int_pair));
		p->first = X;
		p->second = L - 1;
		return p; //! Finished cracking
	}
	while (L <= R) {
		if (c[L] >= a && c[L] < b)
			view[view_size++] = c[L];
		L++;
	}
	return NULL; //! Did not finished cracking
}

IntPair mdd1rp_split_and_materializePredicated(IndexEntry *&c, pair<int64_t, pair<int64_t, int64_t>> &P, int64_t L,
                                               int64_t R, int64_t a, int64_t b, int64_t nswap, IndexEntry *&view,
                                               int64_t &view_size) {
	int64_t X = P.first;
	int64_t &pL = P.second.first, &pR = P.second.second;
	if (view)
		free(view);
	view = (IndexEntry *)malloc((R - L + 1) * sizeof(IndexEntry)); //! initialize view to the maximum possible size

	//! optimization based on where [a,b] relative to X
	if (b < X) {
		assert(pR <= R);
		R = pR; //! skip the right section

		//! fast scan the left
		while (L < pL) {
			int matching = c[L] >= a && c[L] < b;
			view[view_size] = c[L];
			view_size += matching;
			L++;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X) {
				int matching = c[L] >= a && c[L] < b;
				view[view_size] = c[L];
				view_size += matching;
				L++;
			}
			while (L <= R && c[R] >= X)
				R--; //! the arr[R] don't need to be checked for materialization!
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	} else if (X <= a) {
		assert(L <= pL);
		L = pL; //! skip the left section

		//! fast scan the right
		while (pR < R) {
			int matching = c[R] >= a && c[R] < b;
			view[view_size] = c[R];
			view_size += matching;
			R--;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X)
				L++; //! the arr[L] don't need to be checked for materialization!
			while (L <= R && c[R] >= X) {
				int matching = c[R] >= a && c[R] < b;
				view[view_size] = c[R];
				view_size += matching;
				R--;
			}
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	} else {
		//! fast scan left and right
		while (L < pL) {
			int matching = c[L] >= a && c[L] < b;
			view[view_size] = c[L];
			view_size += matching;
			L++;
		}
		while (pR < R) {
			int matching = c[R] >= a && c[R] < b;
			view[view_size] = c[R];
			view_size += matching;
			R--;
		}

		for (; L <= R;) { //! split [L,R) based on X and materialize
			while (L <= R && c[L] < X) {
				int matching = c[L] >= a;
				view[view_size] = c[L];
				view_size += matching;
				L++;
			}
			while (L <= R && c[R] >= X) {
				int matching = c[R] < b;
				view[view_size] = c[R];
				view_size += matching;
				R--;
			}
			if (L < R) {
				exchange(c, L, R);
				if (nswap-- <= 0)
					break;
			}
		}
	}
	pL = L;
	pR = R;

	if (L > R) {
		//! add crack on X at position L
		IntPair p = (IntPair)malloc(sizeof(struct int_pair));
		p->first = X;
		p->second = L - 1;
		return p; //! Finished cracking
	}
	while (L <= R) {
		int matching = c[L] >= a && c[L] < b;
		view[view_size] = c[L];
		view_size += matching;
		L++;
	}
	return NULL; //! Did not finished cracking
}

IntPair mdd1rp_find(IndexEntry *&c, int64_t L, int64_t R, int64_t a, int64_t b, int nswap, IndexEntry *&view,
                    int64_t &view_size, CrackEngineType engineType) {
	IntPair pivot_pair;
	if (R - L < L2_SIZE) { //! full crack if the piece size is less than 1M tuples
		switch (engineType) {
		case CrackEngineType ::Branched: {
			return crackInTwoMDD1RBranched(c, L, R, a, b, view, view_size);
		}
		case CrackEngineType::Predicated: {
			return crackInTwoMDD1RPredicated(c, L, R, a, b, view, view_size);
		}
		case CrackEngineType::Rewired:
			return nullptr;
		}
	}
	if (!partial_crack.count(L)) { //! pick a pivot value X randomly in index [L,R)
		int64_t x = c[L + (1 + (int64_t)((R - L + 1) * (double)rand() / (RAND_MAX + 1.0))) - 1].m_key;
		partial_crack[L] = make_pair(x, make_pair(L, R));
	}
	switch (engineType) {
	case CrackEngineType ::Branched: {
		pivot_pair = mdd1rp_split_and_materializeBranched(c, partial_crack[L], L, R, a, b, nswap, view, view_size);
	} break;
	case CrackEngineType::Predicated: {
		pivot_pair = mdd1rp_split_and_materializePredicated(c, partial_crack[L], L, R, a, b, nswap, view, view_size);
	} break;

	case CrackEngineType::Rewired:
		break;
	}
	//!    pivot_pair = mdd1rp_split_and_materializeBranched(c, partial_crack[L], L, R, a, b, nswap,
	//!    view, view_size);
	if (pivot_pair) {
		assert(partial_crack.count(L));
		partial_crack.erase(L); //! remove from partial crack
	}
	return pivot_pair;
}

AvlTree progressiveStochasticCracking(IndexEntry *&c, int64_t dataSize, AvlTree T, int64_t lowKey, int64_t highKey,
                                      QueryOutput *qo, CrackEngineType engineType) {

	IntPair p1, p2;

	p1 = FindNeighborsLT(lowKey, T, dataSize - 1);
	p2 = FindNeighborsLT(highKey, T, dataSize - 1);

	IntPair pivot_pair = NULL;

	if (p1->first == p2->first && p1->second == p2->second) {
		pivot_pair = mdd1rp_find(c, p1->first, p1->second, lowKey, highKey, COLUMN_SIZE * ALLOWED_SWAPS_PERCENTAGE,
		                         qo->view1, qo->view_size1,
		                         engineType); //! a = L b = R N = Column_size p = swap_percentage
		if (pivot_pair) {
			lowKey = pivot_pair->first;
			highKey = pivot_pair->first;
			pivot_pair->first = pivot_pair->second;
		}
	} else {
		pivot_pair = (IntPair)malloc(sizeof(struct int_pair));
		pivot_pair->first = 0;
		pivot_pair->second = 0;
		IntPair pivot_pair1 =
		    mdd1rp_find(c, p1->first, p1->second, lowKey, highKey, COLUMN_SIZE * ALLOWED_SWAPS_PERCENTAGE, qo->view1,
		                qo->view_size1, engineType);
		qo->middlePart = &c[p1->second + 1];
		int size2 = p2->first - p1->second - 1;
		qo->middlePart_size = size2;
		IntPair pivot_pair2 =
		    mdd1rp_find(c, p2->first, p2->second, lowKey, highKey, COLUMN_SIZE * ALLOWED_SWAPS_PERCENTAGE, qo->view2,
		                qo->view_size2, engineType);

		if (pivot_pair1 && pivot_pair2) {
			pivot_pair->first = pivot_pair1->second;
			lowKey = pivot_pair1->first;
			pivot_pair->second = pivot_pair2->second;
			highKey = pivot_pair2->first;

		} else if (pivot_pair1) {
			lowKey = pivot_pair1->first;
			highKey = pivot_pair1->first;
			pivot_pair1->first = pivot_pair1->second;
			pivot_pair = pivot_pair1;
		} else if (pivot_pair2) {
			lowKey = pivot_pair2->first;
			highKey = pivot_pair2->first;
			pivot_pair2->first = pivot_pair2->second;
			pivot_pair = pivot_pair2;
		}

		free(pivot_pair1);
		free(pivot_pair2);
	}

	if (pivot_pair) {
		if (pivot_pair->first) {
			T = Insert(pivot_pair->first, lowKey, T);
		}

		if (pivot_pair->second) {
			T = Insert(pivot_pair->second, highKey, T);
		}
		pivot_pair->first = 0;
		pivot_pair->second = 0;
		pivot_pair = NULL;
	}

	free(p1);
	free(p2);
	return T;
}
