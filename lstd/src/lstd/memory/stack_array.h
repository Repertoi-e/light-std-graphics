#pragma once

#include "../types.h"
#include "array_like.h"
#include "qsort.h"
#include "string_utils.h"

LSTD_BEGIN_NAMESPACE

template <typename T>
struct array;

//
// A wrapper around T arr[..] which makes it easier to pass around and work with.
//
// To make an array from a list of elements use:
//
//  auto arr1 = to_stack_array(1, 4, 9);
//  auto arr2 = to_stack_array<s64>(1, 4, 9);
//
// To iterate:
// For(arr1) {
//     ...
// }
//
// For(range(arr1.Count)) {
//     T element = arr1[it];
//     ...
// }
//
// Different from array<T>, because the latter supports dynamic resizing.
// This object contains no other member than T Data[N], _Count_ is a static member for the given type and doesn't take space,
// which means that sizeof(stack_array<T, N>) == sizeof(T) * N.
//
// :CodeReusability: This is considered array_like (take a look at array_like.h)
template <typename T_, s64 N>
struct stack_array {
    using T = T_;

    T Data[N ? N : 1];
    static constexpr s64 Count = N;

    //
    // Iterators:
    //
    using iterator       = T *;
    using const_iterator = const T *;

    constexpr iterator begin() { return Data; }
    constexpr iterator end() { return Data + Count; }
    constexpr const_iterator begin() const { return Data; }
    constexpr const_iterator end() const { return Data + Count; }

    //
    // Operators:
    //
    operator array<T>() const;

    constexpr T &operator[](s64 index) { return Data[translate_index(index, Count)]; }
    constexpr const T &operator[](s64 index) const { return Data[translate_index(index, Count)]; }
};

namespace internal {
template <typename D, typename...>
struct return_type_helper {
    using type = D;
};

template <typename... Types>
struct return_type_helper<void, Types...> : types::common_type<Types...> {
};

template <class T, s64 N, s64... I>
constexpr stack_array<types::remove_cv_t<T>, N> to_array_impl(T (&a)[N], integer_sequence<I...>) {
    return {{a[I]...}};
}
}  // namespace internal

template <typename D = void, class... Types>
constexpr stack_array<typename internal::return_type_helper<D, Types...>::type, sizeof...(Types)> to_stack_array(Types &&...t) {
    return {(Types &&) t...};
}

template <typename T, s64 N>
constexpr stack_array<types::remove_cv_t<T>, N> to_stack_array(T (&a)[N]) {
    return internal::to_array_impl(a, make_integer_sequence<N>{});
}

LSTD_END_NAMESPACE
