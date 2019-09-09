#pragma once

#include <assert.h>

class Random {
	static const long long multiplier = 0x5DEECE66DLL;
	static const long long addend = 0xBLL;
	static const long long mask = (1LL << 48) - 1;
	unsigned long long seed;

public:
	Random() {
		setSeed(140384);
	}
	Random(long long s) {
		setSeed(s);
	}
	void setSeed(long long s) {
		seed = (s ^ multiplier) & mask;
	}
	int next(int bits) {
		seed = (seed * multiplier + addend) & mask;
		return (int)(seed >> (48 - bits));
	}
	int nextInt(int n) {
		assert(n > 0);
		//! i.e., n is a power of 2
		if ((n & -n) == n)
			return (int)((n * (long long)next(31)) >> 31);
		int bits, val;
		do {
			bits = next(31);
			val = bits % n;
		} while (bits - val + (n - 1) < 0);
		assert(val >= 0 && val < n);
		return val;
	}
	int nextInt() {
		return next(32);
	}
	long long nextLong() {
		return ((long long)(next(32)) << 32) + next(32);
	}
	bool nextBoolean() {
		return next(1) != 0;
	}
	float nextFloat() {
		return next(24) / ((float)(1 << 24));
	}
	double nextDouble() {
		return (((long long)(next(26)) << 27) + next(27)) / (double)(1LL << 53);
	}
};
