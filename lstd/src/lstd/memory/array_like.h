#pragma once

#include "../common/common.h"
#include "../common/type_info.h"

//
// :CodeReusability: This file implements:
//  * get, get_subarray, find, find_not, find_any_of, find_not_any_of, has, compare, compare_lexicographically
// and operators ==, !=, <, <=, >, >=
//
// for structures that have members Data and Count - we call these array-likes.
//
// e.g. stack_array, array, string, delegate, guid, all have the members Data and Count, so functions
// are automatically generated to all three types using the definition below.
// They also resolves for mismatching types, e.g. stack_array != array, etc. for which otherwise
// we need a combinatorial amount of code to support.
//
// Note: For arrays of different data types comparing doesn't reinterpret the values
// but directly returns false. e.g. array<char> is ALWAYS != array<s8>.
//
// Your custom types (which aren't explicitly flagged) will also automatically get this treatment.
// You can explicitly disable this with a member "static constexpr bool IS_ARRAY = false;"
//
// Note: We currently expect the members to be named exactly Data and Count which may conflict with other naming styles.
// How should we handle this?
//

LSTD_BEGIN_NAMESPACE

template <typename T>
concept array_has_flag = requires(T t) {
    {t.IS_ARRAY};
};

template <typename T>
concept array_flag_false = requires(T t) {
    {t.IS_ARRAY == false};
};

template <typename T>
concept array_has_members = requires(T t) {
    {t.Data};
    {t.Count};
};

template <typename T>
concept is_array_like_helper = array_has_members<T> &&(array_has_flag<T> && !array_flag_false<T> || !array_has_flag<T>);

// True if the type has _Data_ and _Count_ members (and the optional explicit flag is not false).
template <typename T>
concept is_array_like = is_array_like_helper<types::remove_cvref_t<T>>;

// This returns the type of the _Data_ member of an array-like object
template <is_array_like T>
using array_data_t = types::remove_pointer_t<decltype(T::Data)>;

//
// This function translates an index that may be negative to an actual index.
// For example 5 maps to 5
// but -5 maps to length - 5
//
// It is used to support Python-like negative indexing.
//
// This function checks if the index is in range.
//
// If _toleratePastLast_ is true, an index == length is accepted.
// This is useful for when you want to translate the end index of a range -
// in that case the end index is exclusive, so index == length is fine.
constexpr always_inline s64 translate_index(s64 index, s64 length, bool toleratePastLast = false) {
    s64 checkLength = toleratePastLast ? length + 1 : length;

    if (index < 0) {
        s64 actual = length + index;
        assert(actual >= 0);
        assert(actual < checkLength);
        return actual;
    }
    assert(index < checkLength);
    return index;
}

//
// Search functions for arrays:
//

//
// @Speed Optimize these functions for scalars (using bit hacks)
//

template <typename T>
struct delegate;

template <is_array_like T>
constexpr auto get(T *arr, s64 index) { return &arr.Data[translate_index(index, arr->Count)]; }

// This may remove const from _arr_
template <is_array_like T>
constexpr auto get_subarray(T *arr, s64 begin, s64 end) {
    s64 targetBegin = translate_index(begin, arr->Count);
    s64 targetEnd   = translate_index(end, arr->Count, true);

    types::remove_cvref<T> result;
    result.Data  = arr.Data + targetBegin;
    result.Count = targetEnd - targetBegin + 1;
    return result;
}

// Find the first occurence of an element which matches the predicate and is after a specified index.
// Predicate must take a single argument (the current element) and return if it matches.
template <is_array_like T>
constexpr s64 find(const T &arr, const delegate<bool(array_data_t<T> &)> &predicate, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) if (predicate(arr.Data[it])) return it;
    return -1;
}

// Find the first occurence of an element that is after a specified index
template <is_array_like T>
constexpr s64 find(const T &arr, const array_data_t<T> &element, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) if (arr.Data[it] == element) return it;
    return -1;
}

// Find the first occurence of a subarray that is after a specified index
template <is_array_like T>
constexpr s64 find(const T &arr, const T &arr2, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    if (!arr2.Data || arr2.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) {
        auto progress = arr2.Data;
        for (auto search = arr.Data + it; progress != arr2.Data + arr2.Count; ++search, ++progress) {
            if (*search != *progress) break;
        }
        if (progress == arr2.end()) return it;
    }
    return -1;
}

// Find the first occurence of any element in the specified subarray that is after a specified index
template <is_array_like T>
constexpr s64 find_any_of(const T &arr, const T &allowed, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    if (!allowed.Data || allowed.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) if (allowed.has(arr.Data[it])) return it;
    return -1;
}

// Find the first absence of an element that is after a specified index
template <is_array_like T>
constexpr s64 find_not(const T &arr, const array_data_t<T> &element, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) if (arr.Data[it] != element) return it;
    return -1;
}

// Find the first absence of any element in the specified subarray that is after a specified index
template <is_array_like T>
constexpr s64 find_not_any_of(const T &arr, const T &banned, s64 start = 0, bool reversed = false) {
    if (!arr.Data || arr.Count == 0) return -1;
    if (!banned.Data || banned.Count == 0) return -1;
    start = translate_index(start, arr.Count);
    For(range(start, reversed ? -1 : arr.Count, reversed ? -1 : 1)) if (!banned.has(arr.Data[it])) return it;
    return -1;
}

// Checks if _item_ is contained in the array
template <is_array_like T>
constexpr bool has(const T &arr, const array_data_t<T> &item) { return find(arr, item) != -1; }

//
// Compare functions for arrays:
//

template <typename T, typename U>
concept arrays_comparable = requires(T t, U u) { t.Data[0] == u.Data[0]; };

template <typename T, typename U>
concept arrays_lexicographically_comparable = requires(T t, U u) {
    t.Data[0] == u.Data[0];
    t.Data[0] < u.Data[0];
};

// Compares this array to _arr_ and returns the index of the first element that is different.
// If the arrays are equal, the returned value is -1.
template <is_array_like T, is_array_like U>
requires arrays_comparable<T, U>
constexpr s64 compare(const T &arr1, const U &arr2) {
    if ((void *) arr1.Data == (void *) arr2.Data && arr1.Count == arr2.Count) return -1;

    if (!arr1.Count && !arr2.Count) return -1;
    if (!arr1.Count || !arr2.Count) return 0;

    auto *s1   = arr1.Data;
    auto *s2   = arr2.Data;
    auto *end1 = arr1.Data + arr1.Count;
    auto *end2 = arr2.Data + arr2.Count;
    while (*s1 == *s2) {
        ++s1, ++s2;
        if (s1 == end1 && s2 == end2) return -1;
        if (s1 == end1) return s1 - arr1.Data;
        if (s2 == end2) return s2 - arr2.Data;
    }
    return s1 - arr1.Data;
}

// Compares this array to to _arr_ lexicographically.
// The result is -1 if this array sorts before the other, 0 if they are equal, and +1 otherwise.
template <is_array_like T, is_array_like U>
requires arrays_lexicographically_comparable<T, U>
constexpr s32 compare_lexicographically(const T &arr1, const U &arr2) {
    if ((void *) arr1.Data == (void *) arr2.Data && arr1.Count == arr2.Count) return 0;

    if (!arr1.Count && !arr2.Count) return 0;
    if (!arr1.Count) return -1;
    if (!arr2.Count) return 1;

    auto *s1   = arr1.Data;
    auto *s2   = arr2.Data;
    auto *end1 = arr1.Data + arr1.Count;
    auto *end2 = arr2.Data + arr2.Count;
    while (*s1 == *s2) {
        ++s1, ++s2;
        if (s1 == end1 && s2 == end2) return 0;
        if (s1 == end1) return -1;
        if (s2 == end2) return 1;
    }
    return *s1 < *s2 ? -1 : 1;
}

//
// Comparison operators for arrays:
//

// @Cleanup: C++20 space ship operator to reduce this bloat..
// .. except that I tried it and it doesn't work for some reason?
// For now let's implement them separately..

template <is_array_like T, is_array_like U>
requires arrays_comparable<T, U>
constexpr auto operator<=>(const T &arr1, const U &arr2) {
    return compare_lexicographically(arr1, arr2);
}

/*
    template <is_array_like T, is_array_like U>
    requires arrays_comparable<T, U>
constexpr bool operator!=(const T &arr1, const U &arr2) {
    return compare(arr1, arr2) != -1;
}

template <is_array_like T, is_array_like U>
    requires arrays_lexicographically_comparable<T, U>
constexpr bool operator<(const T &arr1, const U &arr2) {
    return compare_lexicographically(arr1, arr2) < 0;
}

template <is_array_like T, is_array_like U>
    requires arrays_lexicographically_comparable<T, U>
constexpr bool operator<=(const T &arr1, const U &arr2) {
    return !(compare_lexicographically(arr1, arr2) > 0);
}

template <is_array_like T, is_array_like U>
    requires arrays_lexicographically_comparable<T, U>
constexpr bool operator>(const T &arr1, const U &arr2) {
    return compare_lexicographically(arr1, arr2) > 0;
}

template <is_array_like T, is_array_like U>
    requires arrays_lexicographically_comparable<T, U>
constexpr bool operator>=(const T &arr1, const U &arr2) {
    return !(compare_lexicographically(arr1, arr2) < 0);
}
*/

LSTD_END_NAMESPACE
