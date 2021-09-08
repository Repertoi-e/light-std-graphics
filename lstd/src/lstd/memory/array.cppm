module;

#include "../common/context.h"

export module lstd.array;

import lstd.stack_array;

LSTD_BEGIN_NAMESPACE

//
// This object contains a typed pointer and a size. Used as a basic wrapper around arrays.
// :CodeReusability: This is considered array_like (take a look at array_like.h).
//
// Functions on this object allow negative reversed indexing which begins at
// the end of the string, so -1 is the last character -2 the one before that, etc. (Python-style)
//
// Note: We have a very fluid philosophy of containers and ownership. We don't implement
// copy constructors or destructors, which means that the programmer is totally in control
// of how the memory gets managed.
// See :TypePolicy in "common.h"
//
// This means that this type can be just an array wrapper (a view) or it can also be used as a dynamic array type.
// It's up to the programmer. It also may point to a buffer that came from a totally different place.
// If this type has allocated (by explictly calling reserve() or any modifying functions which call reserve())
// then the programmer must call free(arr.Data).
//
// free(arr.Data) will crash if the pointer is not a heap allocated block (which should contain an allocation header).
//
// This object being just two 8 bit integers can be cheaply and safely passed to functions without performance
// concerns. In order to get a deep copy use clone().
//
// Note: The defer macro helps with calling free (i.e. defer(free(arr.Data))
// releases an allocated array's data at scope exit.
//

export {
    template <typename T_>
    struct array {
        using T = T_;

        T *Data   = null;
        s64 Count = 0;

        // s64 Allocated = 0; // We now check the allocation header of _Data_ to save space on this structure.

        constexpr array() {}
        constexpr array(T *data, s64 count) : Data(data), Count(count) {}

        constexpr array(const initializer_list<T> &items) {
            // A bug caused by this bit me hard...

            static_assert(false, "Don't create arrays which are views into initializer lists (they get optimized in Release).");
            static_assert(false, "Use dynamic arrays or store the values on the stack - e.g. make_stack_array(1, 4, 9...)");
        }

        //
        // Operators:
        //
        auto operator[](s64 index) { return get(this, index); }
        auto operator[](s64 index) const { return get(this, index); }

        // To check if empty
        constexpr operator bool() { return Count; }

        // Subarray operator:
        //
        // e.g. call like this: arr[{2, -1}]
        //      to get everything from the second to the last element (without the last).
        //
        // We support Python-like negative indices.
        //
        // This doesn't allocate memory but just returns a new data pointer and count.
        struct subarray_indices {
            s64 Begin, End;
        };

        constexpr auto operator[](subarray_indices range) { return get_subarray(this, range.Begin, range.End); }
        constexpr auto operator[](subarray_indices range) const { return get_subarray(this, range.Begin, range.End); }
    };

    // Operator to convert a stack array into an array view
    template <typename T, s64 N>
    stack_array<T, N>::operator array<T>() const { return array<T>((T *) Data, Count); }

    // We need helpers because we can't check templated types with types::is_same...
    template <typename T>
    struct is_array_helper : types::false_t {};
    template <typename T>
    struct is_array_helper<array<T>> : types::true_t {};

    // For the terse C++20 templated function syntax ...
    template <typename T>
    concept is_array = is_array_helper<T>::value;

    // Allocates a buffer (using the Context's allocator by default).
    // If _arr->Count_ is not zero we copy the elements _arr_ is pointing to.
    //
    // When adding elements to a dynamic array it grows automatically.
    // However, coming up with a good value for the initial _n_ can improve performance.
    void make_dynamic_array(is_array auto *arr, s64 n, allocator alloc = {});

    // Returns how many elements fit in _arr->Data_, i.e. the size of the buffer.
    // This is not always == arr->Count. When the buffer fills we resize it to fit more.
    s64 get_allocated(is_array auto *arr);

    // When DEBUG_MEMORY is defined we keep a global list of allocations,
    // so we can check if the array is dynamically allocated.
    //
    // This can catch bugs. Calling routines that
    // insert/remove/modify elements to a non-heap array is dangerous.
    //
    // In Release configurations we don't do this check.
    bool is_dynamically_allocated(is_array auto *arr);

    // Allocates a buffer (using the Context's allocator) and copies the old elements.
    // We try to call realloc which may actually save us an allocation.
    void resize(is_array auto *arr, s64 n);

    // Checks _arr_ if there is space for at least _fit_ new elements.
    // Resizes the array if there is not enough space. The new size is equal to the next
    // power of two bigger than (arr->Count + fit), minimum 8.
    void maybe_grow(is_array auto *arr, s64 fit);

    // Don't free the buffer, just move Count to 0
    void reset(is_array auto *arr) { arr->Count = 0; }

    // Checks if there is enough reserved space for _fit_ elements
    bool has_space_for(is_array auto *arr, s64 fit) { return arr->Count + n <= get_allocated(arr); }

    // Overrides the _index_'th element in the array
    void set(is_array auto *arr, s64 index, auto element);

    // Inserts an element at a specified index and returns a pointer to it in the buffer
    auto *insert_at(is_array auto *arr, s64 index, auto element);

    // Insert a buffer of elements at a specified index
    auto *insert_at(is_array auto *arr, s64 index, auto *ptr, s64 size);

    // Insert an array at a specified index and returns a pointer to the beginning of it in the buffer
    auto *insert_at(is_array auto *arr, s64 index, is_array arr2) { return insert_at(arr, index, arr2.Data, arr2.Count); }

    // Removes element at specified index and moves following elements back
    void remove_at(is_array auto *arr, s64 index);

    // Removes element at specified index and moves the last element to the empty slot.
    // This is faster than remove because it doesn't move everything back
    // but this doesn't keep the order of the elements.
    void remove_at_unordered(is_array auto *arr, s64 index);
    // Removes a range [begin, end) and moves following elements back
    void remove_range(is_array auto *arr, s64 begin, s64 end);

    // Appends an element to the end and returns a pointer to it in the buffer
    auto *add(is_array auto *arr, auto element) { return insert_at(arr, arr->Count, element); }

    // Appends a buffer of elements to the end and returns a pointer to it in the buffer
    auto *add(is_array auto *arr, auto *ptr, s64 size) { return insert_at(arr, arr->Count, ptr, size); }

    // Appends an array to the end and returns a pointer to the beginning of it in the buffer
    auto *add(is_array auto *arr, is_array auto arr2) { return insert_at(arr, arr->Count, arr2); }

    // Replace all occurences of a subarray with another array.
    // @Speed See comments in function.
    void replace_all(is_array auto *arr, is_array auto search, is_array auto replace);

    // Replace all occurences of an element from an array with another element.
    void replace_all(is_array auto *arr, auto search, auto replace) {
        auto s = make_stack_array(search);
        auto r = make_stack_array(replace);
        replace_all(arr, s, r);
    }

    // Replace all occurences of an element from an array with an array.
    void replace_all(is_array auto *arr, auto search, is_array auto replace) {
        auto s = make_stack_array(search);
        replace_all(arr, s, replace);
    }

    // Replace all occurences of a subarray from an array with an element.
    void replace_all(is_array auto *arr, is_array auto search, auto replace) {
        auto r = make_stack_array(replace);
        replace_all(arr, search, r);
    }

    // Removes all occurences of a subarray from an array.
    template <is_array T>
    void remove_all(is_array auto *arr, is_array auto search) {
        replace_all(arr, search, {});  // Replace with an empty array
    }

    // Removes all occurences of an element from an array.
    void remove_all(is_array auto *arr, auto search) {
        auto s = make_stack_array(search);
        remove_all(arr, s);
    }

    // Returns a deep copy of _src_
    auto clone(is_array auto src, allocator alloc = {}) {
        decltype(src) result;
        make_dynamic_array(&result, src.Count, alloc);
        add(&result, src);
        return result;
    }
}

module : private;

void make_dynamic_array(is_array auto *arr, s64 n, allocator alloc = {}) {
    auto *oldData = arr->Data;

    using T = types::remove_pointer_t<decltype(arr->Data)>;

    // If alloc is null we use the Context's allocator
    arr->Data = malloc<T>({.Count = n, .Alloc = alloc});
    if (arr->Count) copy_elements(arr->Data, oldData, arr->Count);
}

s64 get_allocated(is_array auto *arr) {
    using T = types::remove_pointer_t<decltype(arr->Data)>;
    return ((allocation_header *) arr.Data - 1)->Size / sizeof(T);
}

bool is_dynamically_allocated(is_array auto *arr) {
#if defined DEBUG_MEMORY
// @TODO: Do work
#endif
    return true;
}

void resize(is_array auto *arr, s64 n) {
    assert(n >= arr->Count && "There will be no space to fit the old elements");
    arr->Data = realloc(arr->Data, {.NewCount = n});
}

void maybe_grow(is_array auto *arr, s64 fit) {
    assert(is_dynamically_allocated(arr));

    s64 space = get_allocated(arr);

    if (arr->Count + fit <= space) return;

    s64 target = max(ceil_pow_of_2(arr->Count + fit + 1), 8);
    resize(target);
}

void set(is_array auto *arr, s64 index, auto element) {
    using T = types::remove_pointer_t<decltype(arr->Data)>;
    static_assert(types::is_convertible<decltype(element), T>, "Cannot convert element to array type");

    assert(is_dynamically_allocated(arr));

    auto i        = translate_index(index, arr->Count);
    *arr->Data[i] = element;
}

auto *insert_at(is_array auto *arr, s64 index, auto element) {
    using T = types::remove_pointer_t<decltype(arr->Data)>;
    static_assert(types::is_convertible<decltype(element), T>, "Cannot convert element to array type");

    maybe_grow(arr, 1);

    s64 offset  = translate_index(index, arr->Count, true);
    auto *where = arr->Data + offset;
    if (offset < arr->Count) {
        copy_elements(where + 1, where, arr->Count - offset);
    }
    copy_elements(where, &element, 1);
    ++arr->Count;
    return where;
}

auto *insert_at(is_array auto *arr, s64 index, auto *ptr, s64 size) {
    using T = types::remove_pointer_t<decltype(arr->Data)>;
    using U = types::remove_pointer_t<ptr>;
    static_assert(types::is_same<T, U>, "Adding elements of different types");

    maybe_grow(arr, size);

    s64 offset  = translate_index(index, arr->Count, true);
    auto *where = arr->Data + offset;
    if (offset < arr->Count) {
        copy_elements(where + size, where, arr->Count - offset);
    }
    copy_elements(where, ptr, size);
    arr->Count += size;
    return where;
}

void remove_at(is_array auto *arr, s64 index) {
    assert(is_dynamically_allocated(arr));

    s64 offset = translate_index(index, arr->Count);

    auto *where = arr->Data + offset;
    copy_elements(where, where + 1, arr->Count - offset - 1);
    --arr->Count;
}

void remove_at_unordered(is_array auto *arr, s64 index) {
    assert(is_dynamically_allocated(arr));

    s64 offset = translate_index(index, arr->Count);

    auto *where = arr->Data + offset;

    // No need when removing the last element
    if (offset != arr->Count - 1) {
        *where = arr->Data + arr->Count - 1;
    }
    --arr->Count;
}

void remove_range(is_array auto *arr, s64 begin, s64 end) {
    assert(is_dynamically_allocated(arr));

    s64 targetBegin = translate_index(begin, arr->Count);
    s64 targetEnd   = translate_index(end, arr->Count, true);

    auto where    = arr->Data + targetBegin;
    auto whereEnd = arr->Data + targetEnd;

    s64 elementCount = whereEnd - where;
    copy_elements(where, whereEnd, arr->Count - targetBegin - elementCount);
    arr->Count -= elementCount;
}

void replace_all(is_array auto *arr, is_array auto search, is_array auto replace) {
    using T = types::remove_pointer_t<decltype(arr->Data)>;
    using U = types::remove_pointer_t<decltype(arr.search)>;
    using V = types::remove_pointer_t<decltype(replace.search)>;
    static_assert(types::is_same<T, U>, "Searching for a subarray of different type");
    static_assert(types::is_same<T, V>, "Replacement elements of different type");

    if (!arr->Data || !arr->Count) return;

    assert(search.Data && search.Count);
    if (replace.Count) assert(replace.Data);

    if (search.Count == replace.Count) {
        // This case we can handle relatively fast.
        // @Speed Improve by using bit hacks for the case when the elements are less than a pointer size?
        auto *p = arr->Data;
        auto *e = arr->Data + arr->Count;
        while (p != e) {
            // @Speed We can do simply compare_memory for scalar types and types that don't have overloaded ==.
            if (p == search[0]) {
                auto *n  = p;
                auto *sp = search.Data;
                auto *se = search.Data + search.Count;
                while (n != e && sp != se) {
                    // Require only operator == to be defined (and not !=).
                    if (!(*p == *sp)) break;
                }

                if (sp == se) {
                    // Match found
                    copy_elements(p, replace.Data, replace.Count);
                    p += replace.Count;
                } else {
                    ++p;
                }
            } else {
                ++p;
            }
        }
    } else {
        // @Speed This is a slow and dumb version for now.
        // We can improve performance (at the cost of space) by either:
        // * Allocating a buffer first which holds the result
        // * Doing two passes, first one counting the number of occurences
        //   so we know the offsets for the second pass.
        // Though the second option would only work if search.Count > replace.Count.
        //
        // I think going with the former makes the most sense,
        // however at that point letting the caller write their own routine
        // will probably be better, since we can't for sure know the context.

        s64 diff = replace.Count - search.Count;

        s64 i = 0;
        while (i < arr->Count && (i = find(arr, search, i)) != -1) {
            if (diff > 0) {
                maybe_grow(arr, diff);
            }

            auto *data = arr->Data + i;

            // Make space for the new elements
            copy_elements(data + replace.Count, data + search.Count, arr->Count - i - arr2.Count);

            // Copy replace elements
            copy_elements(data, replace.Data, replace.Count);

            arr->Count += diff;

            i += replace.Count;
        }
    }
}
}

LSTD_END_NAMESPACE
