#ifndef WHIST_ATOMIC_H
#define WHIST_ATOMIC_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)

// We have C11 atomics, use them.
#include <stdatomic.h>

#elif defined(_MSC_VER)

// This is a simple implementation of the default sequentially-consistent
// C11 atomic operations on int using the MSVC Interlocked* functions.
// (It works on MSVC LONG, which  is 32-bit like int elsewhere.)

// This is a structure type to allow proper type checking.
typedef struct {
    LONG value;
} atomic_int;

#define ATOMIC_VAR_INIT(value) \
    { value }

static inline void atomic_init(volatile atomic_int *object, int value) { object->value = value; }

static inline int atomic_load(volatile atomic_int *object) {
    // Windows guarantees that simple loads and stores to aligned word-size
    // variables are atomic (that is: the compiler doesn't do anything funny
    // and Windows doesn't run on platforms where that wouldn't be sufficient).
    // For loads, this is fine.
    return object->value;
}

static inline void atomic_store(volatile atomic_int *object, int desired) {
    // For stores, it's not enough for sequentially-consistent atomics
    // because loads can be reordered across it.  Use an exchange to
    // ensure we have the expected behaviour.
    InterlockedExchange(&object->value, desired);
}

static inline int atomic_exchange(volatile atomic_int *object, int desired) {
    return InterlockedExchange(&object->value, desired);
}

static inline int atomic_compare_exchange_strong(volatile atomic_int *object, int *expected,
                                                 int desired) {
    // The behaviour isn't quite the same here - MSVC does not overwrite the
    // expected value if the exchange fails, so we need a little more code
    // to make that happen.
    LONG tmp = *expected;
    LONG result = InterlockedCompareExchange(&object->value, desired, tmp);
    if (result == tmp) {
        return true;
    } else {
        *expected = result;
        return false;
    }
}

static inline int atomic_compare_exchange_weak(volatile atomic_int *object, int *expected,
                                               int desired) {
    // MSVC has no weak version, so just call the strong one.
    return atomic_compare_exchange_strong(object, expected, desired);
}

static inline int atomic_fetch_add(volatile atomic_int *object, int operand) {
    return InterlockedExchangeAdd(&object->value, operand);
}

static inline int atomic_fetch_sub(volatile atomic_int *object, int operand) {
    // No separate atomic subtract, so negate the value and add it.
    return InterlockedExchangeAdd(&object->value, -operand);
}

static inline int atomic_fetch_or(volatile atomic_int *object, int operand) {
    return InterlockedOr(&object->value, operand);
}

static inline int atomic_fetch_xor(volatile atomic_int *object, int operand) {
    return InterlockedXor(&object->value, operand);
}

static inline int atomic_fetch_and(volatile atomic_int *object, int operand) {
    return InterlockedAnd(&object->value, operand);
}

#elif defined(__cplusplus) && __cplusplus >= 201103L

// We are compiling C++, so we can use C++11 atomics with the same
// semantics as C11.  Note that this section is after the MSVC atomics in
// this header so that the MSVC atomics do get tested when compiled with
// C++ as the protocol tests are.
#include <atomic>

// Allow using atomic_int without a namespace, as in C.
using std::atomic_int;

#else /* No C11 atomics, MSVC atomics or C++11 atomics. */

// cppcheck-suppress preprocessorErrorDirective
#error "No atomic support found."

#endif

#endif /* WHIST_ATOMIC_H */
