#ifndef PROGRESSIVEINDEXING_ASSERT_H
#define PROGRESSIVEINDEXING_ASSERT_H

#include <cstdio>
#include <cstdlib>


/*
 * This file defines the macro `assert()` similar to 'assert.h', yet allows to provide an optional message.
 */

inline void _assert(const bool pred, const char *filename, const unsigned line, const char *predstr, const char *msg)
{
    if (pred) return;

    fflush(stdout);
    fprintf(stderr, "%s:%u: Assertion '%s' failed.", filename, line, predstr);
    if (msg)
        fprintf(stderr, "  %s.", msg);
    fputc('\n', stderr);
    fflush(stderr);

    abort();
}

#ifndef NDEBUG
#define _ASSERT2(PRED, MSG) _assert((PRED), __FILE__, __LINE__, #PRED, MSG)
#else
#define _ASSERT2(PRED, MSG)
#endif

#define _ASSERT1(PRED) _ASSERT2(PRED, nullptr)
#define _GET_ASSERT(_1, _2, NAME, ...) NAME
#define assert(...) _GET_ASSERT(__VA_ARGS__, _ASSERT2, _ASSERT1, XXX)(__VA_ARGS__)

#endif //PROGRESSIVEINDEXING_ASSERT_H
