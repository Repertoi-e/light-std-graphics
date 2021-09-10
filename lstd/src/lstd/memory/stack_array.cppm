module;

#include "../types.h"
#include "qsort.h"

export module lstd.stack_array;

export import lstd.string.utils;

LSTD_BEGIN_NAMESPACE

template <typename T>
struct array;

template <typename D, typename...>
struct return_type_helper {
    using type = D;
};

template <typename... Types>
struct return_type_helper<void, Types...> : types::common_type<Types...> {
};

//
// A wrapper around T arr[..] which makes it easier to pass around and work with.
//
// To make an array from a list of elements use:
//
//  auto arr1 = make_stack_array(1, 4, 9);
//  auto arr2 = make_stack_array<s64>(1, 4, 9);
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
export {
    template <typename T_, s64 N>
    struct stack_array {
        using T = T_;

        T Data[N ? N : 1];
        static constexpr s64 Count = N;

        //
        // Operators:
        //

        // To check if empty
        constexpr operator bool() { return Count; }

        // Convert to an array view
        operator array<T>() const;

        constexpr auto operator[](s64 index) { return get(this, index); }
        constexpr auto operator[](s64 index) const { return get(this, index); }
    };

    template <typename T>
    concept any_stack_array = types::is_same_template_decayed<T, stack_array<s32, 1>>;

    // To make range based for loops work.
    auto begin(any_stack_array auto &arr) { return arr.Data; }
    auto end(any_stack_array auto &arr) { return arr.Data + arr.Count; }

    template <typename D = void, class... Types>
    constexpr stack_array<typename return_type_helper<D, Types...>::type, sizeof...(Types)> make_stack_array(Types && ...t);

    template <typename T, s64 N>
    constexpr stack_array<types::remove_cv_t<T>, N> make_stack_array(T(&a)[N]);
}

template <class T, s64 N, s64... I>
constexpr stack_array<types::remove_cv_t<T>, N> to_array_impl(T (&a)[N], integer_sequence<I...>) {
    return {{a[I]...}};
}

template <typename D, class... Types>
constexpr stack_array<typename return_type_helper<D, Types...>::type, sizeof...(Types)> make_stack_array(Types &&...t) {
    return {(Types &&) t...};
}

template <typename T, s64 N>
constexpr stack_array<types::remove_cv_t<T>, N> make_stack_array(T (&a)[N]) {
    return to_array_impl(a, make_integer_sequence<N>{});
}

LSTD_END_NAMESPACE
