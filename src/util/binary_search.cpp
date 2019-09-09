#include "../include/util/binary_search.h"

void *build_binary_tree(IndexEntry *c, int64_t n) {
	return c;
}

int64_t binary_search(IndexEntry *c, int64_t key, int64_t lower, int64_t upper, bool *foundKey) {

	*foundKey = false;
	while (lower <= upper) {
		int middle = (lower + upper) / 2;
		auto middleElement = c[middle];

		if (middleElement < key) {
			lower = middle + 1;
		} else if (middleElement > key) {
			upper = middle - 1;
		} else {
			*foundKey = true;
			return middle;
		}
	}
	return upper;
}

int64_t binary_search_lt(IndexEntry *c, int64_t key, int64_t start, int64_t end) {
	bool found = false;
	int pos = binary_search(c, key, start, end, &found);
	if (found) {
		while (--pos >= start && c[pos] == key)
			;
	}
	return pos;
}

int64_t binary_search_lte(IndexEntry *c, int64_t key, int64_t start, int64_t end) {
	bool found = false;
	int pos = binary_search(c, key, start, end, &found);

	while (c[pos] <= key)
		pos++;
	pos--;
	return pos;
}

int64_t binary_search_gte(IndexEntry *c, int64_t key, int64_t start, int64_t end) {
	bool found = false;
	int pos = binary_search(c, key, start, end, &found);
	if (found) {
		while (--pos >= start && c[pos] == key)
			;
	}
	++pos;
	return pos;
}

int64_t binary_search(int64_t *c, int64_t key, int64_t lower, int64_t upper, bool *foundKey) {

	*foundKey = false;
	upper--;
	while (lower <= upper) {
		int middle = (lower + upper) / 2;
		auto middleElement = c[middle];

		if (middleElement < key) {
			lower = middle + 1;
		} else if (middleElement > key) {
			upper = middle - 1;
		} else {
			*foundKey = true;
			return middle;
		}
	}
	return upper;
}

int64_t binary_search_lte(int64_t *c, int64_t key, int64_t start, int64_t end) {
	bool found = false;
	int pos = binary_search(c, key, start, end, &found);
	while (pos < end && c[pos] <= key)
		pos++;
	pos--;

	return pos;
}

int64_t binary_search_gte(int64_t *c, int64_t key, int64_t start, int64_t end) {
	bool found = false;
	int pos = binary_search(c, key, start, end, &found);
	if (found) {
		while (--pos >= start && c[pos] == key)
			;
	}
	++pos;
	return pos;
}
