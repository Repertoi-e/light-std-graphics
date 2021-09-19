#pragma once

//
// A header which defines common types, numeric info,
// including bits operations, atomic operations, common math functions,
//     definitions for the macros: assert, defer, For, For_enumerate ...
//     static_for, range,
// Also:
//     copy_memory (memcpy), copy_elements, fill_memory (memset), zero_memory, compare_memory
//   and constexpr variants: const_copy_memory, const_fill_memory, const_zero_memory, const_compare_memory
//
//

#include "common/atomic.h"
#include "common/bits.h"
#include "common/context.h"
#include "common/debug_break.h"
#include "common/defer_assert_for.h"
#include "common/math.h"
#include "common/memory.h"
#include "common/namespace.h"
#include "common/numeric_info.h"
#include "common/sequence.h"
#include "common/u128.h"

//
// Provides replacements for the math functions found in virtually all standard libraries.
// Also provides functions for extended precision arithmetic, statistical functions, physics, astronomy, etc.
// https://www.netlib.org/cephes/
// Note: We don't include everything, just cmath for now.
//       Statistics is a thing we will most definitely include as well in the future.
//       Everything else you can include on your own in your project (we don't want to be bloat-y).
//
// Note: Important difference,
// atan2's return range is 0 to 2PI, and not -PI to PI (as per normal in the C standard library).
//
//
// Parts of the source code that we modified are marked with :WEMODIFIEDCEPHES:
//

/*
Cephes Math Library Release 2.8:  June, 2000
Copyright 1984, 1995, 2000 by Stephen L. Moshier
*/
#include "vendor/cephes/maths_cephes.h"

#define PI 3.1415926535897932384626433832795
#define TAU 6.283185307179586476925286766559

// Convenience storage literal operators, allows for specifying sizes like this:
//  s64 a = 10_MiB;

// _B For completeness
constexpr u64 operator"" _B(u64 i) { return i; }
constexpr u64 operator"" _KiB(u64 i) { return i << 10; }
constexpr u64 operator"" _MiB(u64 i) { return i << 20; }
constexpr u64 operator"" _GiB(u64 i) { return i << 30; }

constexpr u64 operator"" _thousand(u64 i) { return i * 1000; }
constexpr u64 operator"" _million(u64 i) { return i * 1000000; }
constexpr u64 operator"" _billion(u64 i) { return i * 1000000000; }

LSTD_BEGIN_NAMESPACE

// Loop that gets unrolled at compile-time
template <s64 First, s64 Last, typename Lambda>
void static_for(Lambda &&f) {
    if constexpr (First < Last) {
        f(types::integral_constant<s64, First>{});
        static_for<First + 1, Last>(f);
    }
}

// @Volatile: README.md
// :TypePolicy:
//
// Ideal: keep it simple.
//
// - Keep it data oriented.
//   Programs work with data. Design your data so it makes the solution straightforward and minimize abstraction layers.
// - Use struct instead of class, keep everything public.
// - Always provide a default constructor (implicit or by "T() {}")
// - copy/move constructors and destructors are banned. No excuse.
// - No throwing of exceptions, .. ever, .. anywhere. No excuse.
//   They make your code complicated. When you can't handle an error and need to exit from a function, return multiple values.
//   C++ doesn't really help with this, but you can use C++17 structured bindings, e.g.:
//          auto [content, success] = path_read_entire_file("data/hello.txt");
//
//
// Some examples:
//
// _array_ is this library implemented the following way:
//     _array_ is a struct that contains 2 fields (Data and Count).
//
//     It has no sense of ownership. That is determined explictly in code and by the programmer.
//
//     By default arrays are views, to make them dynamic, call     make_dynamic(&arr).
//     After that you can modify them (add/remove elements etc.)
//
//     You can safely pass around copies and return arrays from functions because
//     there is no hidden destructor which will free your memory.
//
//     When a dynamic array is no longer needed call    free(arr.Data);
//
//
//     We provide a defer macro which runs at the end of the scope (like a destructor),
//     you can use this for functions which return from multiple places,
//     so you are absolutely sure    free(arr.Data) is ran and there were no leaks.
//
//
//     All of this allows to skip writing copy/move constructors/assignment operators.
//
// _string_ is just array<u8>. All of this applies to them as well.
// They are not null-terminated, which means that taking substrings doesn't allocate memory.
//
//
//     // Constructed from a zero-terminated string buffer. Doesn't allocate memory.
//     // Like arrays, strings are views by default.
//     string path = "./data/";
//     make_dynamic(&path);         // Allocates a buffer and copies the string it was pointing to
//     defer(free(path.Data));
//
//     append(&path, "output.txt");
//
//     string pathWithoutDot = substring(path, 2, -1);
//
// To make a deep copy of an array use clone().
// e.g.         string newPath = clone(path); // Allocates a new buffer and copies contents in _path_
//

template <typename T>
constexpr void swap(T &a, T &b) {
    T c = a;
    a   = b;
    b   = c;
}

template <typename T, s64 N>
constexpr void swap(T (&a)[N], T (&b)[N]) {
    For(range(N)) swap(a[it], b[it]);
}

//
// copy_memory, fill_memory, compare_memory and SSE optimized implementations when on x86 architecture
// (implemenations in memory/memory.cpp)
//

// In this library, copy_memory works like memmove in the std (handles overlapping buffers)
// @TODO: Merge these using is_constant_evaluated()
extern void *(*copy_memory)(void *dst, const void *src, u64 size);
constexpr void *const_copy_memory(void *dst, const void *src, u64 size) {
    auto *d = (char *) dst;
    auto *s = (const char *) src;

    if (d <= s || d >= (s + size)) {
        // Non-overlapping
        while (size--) {
            *d++ = *s++;
        }
    } else {
        // Overlapping
        d += size - 1;
        s += size - 1;

        while (size--) {
            *d-- = *s--;
        }
    }
    return dst;
}

template <typename T>
T *copy_elements(T *dst, auto *src, s64 n) { return (T *) copy_memory(dst, src, n * sizeof(T)); }

extern void *(*fill_memory)(void *dst, char value, u64 size);
constexpr void *const_fill_memory(void *dst, char value, u64 size) {
    u64 uValue     = (u64) value;
    u64 largeValue = uValue << 56 | uValue << 48 | uValue << 40 | uValue << 32 | uValue << 24 | uValue << 16 | uValue << 8 | uValue;

    u64 offset = ((u64) dst) % sizeof(u64);
    byte *b    = (byte *) dst;
    while (offset--) *b++ = value;

    u64 *dstBig = (u64 *) b;
    u64 bigNum  = (size & (~sizeof(u64) + 1)) / sizeof(u64);
    while (bigNum--) *dstBig++ = largeValue;

    size &= (sizeof(u64) - 1);

    b = (byte *) dstBig;
    while (size--) *b++ = value;
    return dst;
}

inline void *zero_memory(void *dst, u64 size) { return fill_memory(dst, 0, size); }
constexpr void *const_zero_memory(void *dst, u64 size) { return const_fill_memory(dst, 0, size); }

// compare_memory returns the index of the first byte that is different
// e.g: calling with
//		*ptr1 = 0000001234
//		*ptr1 = 0010000234
//	returns 2
// If the memory regions are equal, the returned value is -1
extern s64 (*compare_memory)(const void *ptr1, const void *ptr2, u64 size);
constexpr s64 const_compare_memory(const void *ptr1, const void *ptr2, u64 size) {
    // @TODO: This doesn't work. Complains about casting.
    auto *s1 = (byte *) ptr1;
    auto *s2 = (byte *) ptr2;

    For(range(size)) if (*s1++ != *s2++) return it;
    return -1;
}

LSTD_END_NAMESPACE
