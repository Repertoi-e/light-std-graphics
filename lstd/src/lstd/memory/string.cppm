module;

#include "../memory/allocator.h"

export module lstd.string;

export import lstd.array;

LSTD_BEGIN_NAMESPACE

//
// String doesn't guarantee a null termination at the end.
// It's essentially a data pointer and a count.
//
// This means that you can load a binary file into a string.
//
// The routines defined in array.cppm work with _string_ because
// _string_ is a typedef for array<u8>. However they treat indices
// as pointing to bytes and NOT to code points.
//
// This file provides functions prefixed with string_ which
// treat indices properly (pointing to code points).
// Whenever working with strings we assume valid UTF-8.
// We don't do any checks, that is left up to the programmer to verify.
//
// @TODO: Provide a _string_utf8_validate()_.
//
// Functions on this object allow negative reversed indexing which begins at
// the end of the string, so -1 is the last byte, -2 the one before that, etc. (Python-style)
//
//
// Getting substrings doesn't allocate memory but just returns a new data pointer and count
// since strings in this library are not null-terminated.
//
// The subarray operator can be called like this:    str[{1, 3}];
// which calls _subarray()_ defined in array_like.cppm.
// NOTE: This treats indices as pointing to bytes and NOT code points!
// To get a proper substring call _substring()_, defined in this file.
//
// @TODO: It would be nice to have the same terse substring operator that works for code points...
//

export {
    using string = array<u8>;

    struct code_point_ref {
        string *Parent;
        s64 Index;

        constexpr code_point_ref(string *parent = null, s64 index = -1) : Parent(parent), Index(index) {}

        constexpr code_point_ref &operator=(code_point other);
        constexpr operator code_point() const;
    };

    // The non-const version returns a special structure that allows assigning a new code point.
    // This is legal to do:
    //
    //      string a = "Hello";
    //      string_get(a, 0) = u8'Л';
    //      // a is now "Лello" and contains a two byte code point in the beginning.
    //
    // We need to do this because not all code points are 1 byte.
    //
    // We use a & here because taking a pointer is annoying for the caller.
    constexpr code_point_ref string_get(string & str, s64 index);

    // The const version just returns the decoded code point at that location
    constexpr code_point string_get(const string str, s64 index);

    constexpr code_point_ref &code_point_ref::operator=(code_point other) {
        string_set(Parent, Index, other);
        return *this;
    }

    constexpr code_point_ref::operator code_point() { return string_get((const string *) Parent, Index); }

    constexpr code_point_ref string_get(string & str, s64 index) { return code_point_ref(&str, index); }

    // The const version just returns the decoded code point at that location
    constexpr code_point string_get(const string str, s64 index) {
        if (index < 0) {
            // @Speed... should we cache this in _string_?
            // We need to calculate the total length (in code points)
            // in order for the negative index to be converted properly.
            s64 length = utf8_length(str->Data, str->Count);
            index      = translate_index(index, length);

            // If LSTD_ARRAY_BOUNDS_CHECK is defined:
            // _utf8_get_cp_at_index()_ also checks for out of bounds but we are sure the index is valid
            // (since translate_index also checks for out of bounds) so we can get away with the unsafe version here.
            auto *s = str.Data;
            For(range(index)) s += utf8_get_size_of_cp(str);
            return utf8_decode_cp(s);
        } else {
            auto *s = utf8_get_pointer_to_cp_at_translated_index(str->Data, str->Count, index);
            return utf8_decode_cp(s);
        }
    }

    // Changes the code point at _index_ to a new one. May allocate and change the byte count of the string.
    void string_set(string * str, s64 index, code_point cp) {
        const char *target = utf8_get_pointer_to_cp_at_translated_index(str->Data, str->Count, index);

        char encodedCp[4];
        utf8_encode_cp(encodedCp, cp);

        replace_range(str, target - str->Data, target - str->Data + utf8_get_size_of_cp(target), array<char>(endodedCp, utf8_get_size_of_cp(cp)));
    }

    // Doesn't allocate memory, strings in this library are not null-terminated.
    // We allow negative reversed indexing which begins at the end of the string,
    // so -1 is the last code point, -2 is the one before that, etc. (Python-style)
    constexpr string substring(string str, s64 begin, s64 end) {
        s64 length     = utf8_length(str.Data, str.Count);
        s64 beginIndex = translate_index(begin, length);
        s64 endIndex   = translate_index(end, length, true);

        const char *beginPtr = utf8_get_cp_at_index_unsafe(str.Data, beginIndex);
        const char *endPtr   = beginPtr;

        // @Speed
        For(range(beginIndex, endIndex)) endPtr += utf8_get_size_of_cp(endPtr);

        return string(beginPtr, endPtr - beginPtr);
    }

    template <bool Const>
    struct string_iterator {
        using string_t = types::select_t<Const, const string, string>;

        string_t *Parent;
        s64 Index;

        string_iterator(string_t *parent = null, s64 index = 0) : Parent(parent), Index(index) {}

        string_iterator &operator++() { return *this += 1; }
        string_iterator operator++(s32) {
            string_iterator temp = *this;
            return ++(*this), temp;
        }

        auto operator<=>(string_iterator other) const { return Index <=> other.Index; };

        auto operator*() { return (*Parent)[Index]; }

        operator const char *() const { return utf8_get_cp_at_index_unsafe(Parent->Data, translate_index(Index, Parent->Length, true)); }
    };
}

/*
   public:
    using iterator = string_iterator<false>;
    using const_iterator = string_iterator<true>;

    // To make range based for loops work.
    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, Length); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, Length); }

    //
    // Operators:
    //

    // Returns true if the string contains any code points
    constexpr explicit operator bool() const { return Length; }

    constexpr operator array<utf8>() const { return array<utf8>(Data, Count); }
    constexpr operator array<byte>() const { return array<byte>((byte *) Data, Count); }

    struct code_point_ref {
        string *Parent = null;
        s64 Index = -1;

        code_point_ref() {}
        code_point_ref(string *parent, s64 index) : Parent(parent), Index(index) {}

        code_point_ref &operator=(code_point other);
        operator code_point() const;
    };

    // The non-const version allows to modify the character by simply =.
    // This may allocate because UTF-8 code points are not a const amount of bytes.
    //
    // We support Python-like negative indices.
    code_point_ref operator[](s64 index) { return code_point_ref(this, translate_index(index, Length)); }
    constexpr code_point operator[](s64 index) const { return utf8_decode_cp(utf8_get_cp_at_index_unsafe(Data, translate_index(index, Length))); }

    //
    // Substring operator:
    //
    // e.g. call like this: str[{2, -1}]     
    //      to get everything from the second to the last character (without the last).
    // 
    // We support Python-like negative indices.
    //
    // This doesn't allocate memory but just returns a new data pointer and count
    // since strings in this library are not null-terminated.
    struct substring_indices {
        s64 b, e;
    };
    constexpr string operator[](substring_indices range) const;
};
    */

// Make sure you call the string_ overloads because array_ functions don't calculate the Length (which we cache).
inline void string_reserve(string &s, s64 n) { array_reserve(s, n); }

inline void string_reset(string &s) { array_reset(s), s.Length = 0; }
inline void free(string &s) { free((array<utf8> &) s), s.Length = 0; }

//
// Utilities to convert to c-style strings.
// Functions for conversion between utf8, wchar and code_point are provided in string_utils.h
//

// Allocates a buffer, copies the string's contents and also appends a zero terminator.
// Uses the Context's current allocator. The caller is responsible for freeing.
[[nodiscard("Leak")]] char *string_to_c_string(const string &s, allocator alloc = {});

// Allocates a buffer, copies the string's contents and also appends a zero terminator.
// Uses the temporary allocator.
char *string_to_c_string_temp(const string &s);

//
// String modification:
//

// Sets the _index_'th code point in the string.
void string_set(string &s, s64 index, code_point codePoint);

// Insert a code point at a specified index.
void string_insert_at(string &s, s64 index, code_point codePoint);

// Insert a buffer of bytes at a specified index.
void string_insert_at(string &s, s64 index, const char *str, s64 size);

// Insert a string at a specified index.
inline void string_insert_at(string &s, s64 index, const string &str) { return string_insert_at(s, index, str.Data, str.Count); }

// Remove the first occurence of a code point.
void string_remove(string &s, code_point cp);

// Remove code point at specified index.
void string_remove_at(string &s, s64 index);

// Remove a range of code points. [begin, end)
void string_remove_range(string &s, s64 begin, s64 end);

// Append a non encoded character to a string.
inline void string_append(string &s, code_point codePoint) { return string_insert_at(s, s.Length, codePoint); }

// Append _size_ bytes of string contained in _data_.
inline void string_append(string &s, const char *str, s64 size) { return string_insert_at(s, s.Length, str, size); }

// Append one string to another.
inline void string_append(string &s, const string &str) { return string_append(s, str.Data, str.Count); }

// Replace all occurences of _oldStr_ with _newStr_
inline void string_replace_all(string &s, const string &oldStr, const string &newStr) {
    array_replace_all(s, oldStr, newStr);
    s.Length = utf8_length(s.Data, s.Count);  // @Speed. @TODO Make replace/remove functions return an integer that says how much we've removed
}

// Replace all occurences of _oldCp_ with _newCp_
inline void string_replace_all(string &s, code_point oldCp, code_point newCp) {
    utf8 encodedOld[4];
    utf8_encode_cp(encodedOld, oldCp);

    utf8 encodedNew[4];
    utf8_encode_cp(encodedNew, newCp);

    string_replace_all(s, string(encodedOld, utf8_get_size_of_cp(encodedOld)), string(encodedNew, utf8_get_size_of_cp(encodedNew)));
}

// Removes all occurences of _cp_
inline void string_remove_all(string &s, code_point cp) {
    utf8 encodedCp[4];
    utf8_encode_cp(encodedCp, cp);

    string_replace_all(s, string(encodedCp, utf8_get_size_of_cp(encodedCp)), "");
}

// Remove all occurences of _str_
inline void string_remove_all(string &s, const string &str) { string_replace_all(s, str, ""); }

// Replace all occurences of _oldCp_ with _newStr_
inline void string_replace_all(string &s, code_point oldCp, const string &newStr) {
    utf8 encodedCp[4];
    utf8_encode_cp(encodedCp, oldCp);

    string_replace_all(s, string(encodedCp, utf8_get_size_of_cp(encodedCp)), newStr);
}

// Replace all occurences of _oldStr_ with _newCp_
inline void string_replace_all(string &s, const string &oldStr, code_point newCp) {
    utf8 encodedCp[4];
    utf8_encode_cp(encodedCp, newCp);

    string_replace_all(s, oldStr, string(encodedCp, utf8_get_size_of_cp(encodedCp)));
}

//
// Comparison and searching:
//

// Compares two utf8 encoded strings and returns the index
// of the code point at which they are different or _-1_ if they are the same.
constexpr s64 compare(const string &s, const string &other) {
    if (!s && !other) return -1;
    if (!s || !other) return 0;

    auto *p1 = s.Data, *p2 = other.Data;
    auto *e1 = p1 + s.Count, *e2 = p2 + other.Count;

    s64 index = 0;
    while (utf8_decode_cp(p1) == utf8_decode_cp(p2)) {
        p1 += utf8_get_size_of_cp(p1);
        p2 += utf8_get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return -1;
        if (p1 == e1 || p2 == e2) return index;
        ++index;
    }
    return index;
}

// Compares two utf8 encoded strings while ignoring case and returns the index
// of the code point at which they are different or _-1_ if they are the same.
constexpr s64 compare_ignore_case(const string &s, const string &other) {
    if (!s && !other) return -1;
    if (!s || !other) return 0;

    auto *p1 = s.Data, *p2 = other.Data;
    auto *e1 = p1 + s.Count, *e2 = p2 + other.Count;

    s64 index = 0;
    while (to_lower(utf8_decode_cp(p1)) == to_lower(utf8_decode_cp(p2))) {
        p1 += utf8_get_size_of_cp(p1);
        p2 += utf8_get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return -1;
        if (p1 == e1 || p2 == e2) return index;
        ++index;
    }
    return index;
}

// Compares two utf8 encoded strings lexicographically and returns:
//  -1 if _a_ is before _b_
//   0 if a == b
//   1 if _b_ is before _a_
constexpr s32 compare_lexicographically(const string &a, const string &b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    auto *p1 = a.Data, *p2 = b.Data;
    auto *e1 = p1 + a.Count, *e2 = p2 + b.Count;

    s64 index = 0;
    while (utf8_decode_cp(p1) == utf8_decode_cp(p2)) {
        p1 += utf8_get_size_of_cp(p1);
        p2 += utf8_get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return 0;
        if (p1 == e1) return -1;
        if (p2 == e2) return 1;
        ++index;
    }
    return ((s64) utf8_decode_cp(p1) - (s64) utf8_decode_cp(p2)) < 0 ? -1 : 1;
}

// Compares two utf8 encoded strings lexicographically while ignoring case and returns:
//  -1 if _a_ is before _b_
//   0 if a == b
//   1 if _b_ is before _a_
constexpr s32 compare_lexicographically_ignore_case(const string &a, const string &b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    auto *p1 = a.Data, *p2 = b.Data;
    auto *e1 = p1 + a.Count, *e2 = p2 + b.Count;

    s64 index = 0;
    while (to_lower(utf8_decode_cp(p1)) == to_lower(utf8_decode_cp(p2))) {
        p1 += utf8_get_size_of_cp(p1);
        p2 += utf8_get_size_of_cp(p2);
        if (p1 == e1 && p2 == e2) return 0;
        if (p1 == e1) return -1;
        if (p2 == e2) return 1;
        ++index;
    }
    return ((s64) to_lower(utf8_decode_cp(p1)) - (s64) to_lower(utf8_decode_cp(p2))) < 0 ? -1 : 1;
}

// Searches for the first occurence of a substring which is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_substring(const string &haystack, const string &needle, s64 start = 0) {
    assert(needle.Data && needle.Length);

    if (haystack.Length == 0) return -1;

    if (start >= haystack.Length || start <= -haystack.Length) return -1;

    auto *p   = utf8_get_cp_at_index_unsafe(haystack.Data, translate_index(start, haystack.Length));
    auto *end = haystack.Data + haystack.Count;

    auto *needleEnd = needle.Data + needle.Count;

    while (p != end) {
        while (end - p > 4) {
            if (U32_HAS_BYTE(*(u32 *) p, *needle.Data)) break;
            p += 4;
        }

        while (p != end) {
            if (*p == *needle.Data) break;
            ++p;
        }

        if (p == end) return -1;

        auto *search   = p + 1;
        auto *progress = needle.Data + 1;
        while (search != end && progress != needleEnd && *search == *progress) ++search, ++progress;
        if (progress == needleEnd) return utf8_length(haystack.Data, p - haystack.Data);
        ++p;
    }
    return -1;
}

// Searches for the first occurence of a code point which is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp(const string &haystack, code_point cp, s64 start = 0) {
    utf8 encoded[4]{};
    utf8_encode_cp(encoded, cp);
    return find_substring(haystack, string(encoded, utf8_get_size_of_cp(encoded)), start);
}

// Searches for the last occurence of a substring which is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_substring_reverse(const string &haystack, const string &needle, s64 start = 0) {
    assert(needle.Data && needle.Length);

    if (haystack.Length == 0) return -1;

    if (start >= haystack.Length || start <= -haystack.Length) return -1;
    if (start == 0) start = haystack.Length;

    auto *p   = utf8_get_cp_at_index_unsafe(haystack.Data, translate_index(start, haystack.Length, true) - 1);
    auto *end = haystack.Data + haystack.Count;

    auto *needleEnd = needle.Data + needle.Count;

    while (p > haystack.Data) {
        while (p - haystack.Data > 4) {
            if (U32_HAS_BYTE(*((u32 *) (p - 3)), *needle.Data)) break;
            p -= 4;
        }

        while (p != haystack.Data) {
            if (*p == *needle.Data) break;
            --p;
        }

        if (*p != *needle.Data && p == haystack.Data) return -1;

        auto *search   = p + 1;
        auto *progress = needle.Data + 1;
        while (search != end && progress != needleEnd && *search == *progress) ++search, ++progress;
        if (progress == needleEnd) return utf8_length(haystack.Data, p - haystack.Data);
        --p;
    }
    return -1;
}

// Searches for the last occurence of a code point which is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp_reverse(const string &haystack, code_point cp, s64 start = 0) {
    utf8 encoded[4]{};
    utf8_encode_cp(encoded, cp);
    return find_substring_reverse(haystack, string(encoded, utf8_get_size_of_cp(encoded)), start);
}

// Searches for the first occurence of a substring which is different from _eat_ and is after a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_substring_not("../../../../user/stuff", "../")
//   returns:                        ^
//
constexpr s64 find_substring_not(const string &s, const string &eat, s64 start = 0) {
    assert(eat.Data && eat.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    auto *p   = utf8_get_cp_at_index_unsafe(s.Data, translate_index(start, s.Length));
    auto *end = s.Data + s.Count;

    auto *eatEnd = eat.Data + eat.Count;
    while (p != end) {
        while (p != end) {
            if (*p != *eat.Data) break;
            ++p;
        }

        if (p == end) return -1;

        auto *search   = p + 1;
        auto *progress = eat.Data + 1;
        while (search != end && progress != eatEnd && *search != *progress) ++search, ++progress;
        if (progress == eatEnd) return utf8_length(s.Data, p - s.Data);
        ++p;
    }
    return -1;
}

// Searches for the first occurence of a code point which is not _cp_ and is after a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_cp_not(const string &s, code_point cp, s64 start = 0) {
    utf8 encoded[4]{};
    utf8_encode_cp(encoded, cp);
    return find_substring_not(s, string(encoded, utf8_get_size_of_cp(encoded)), start);
}

// Searches for the last occurence of a substring which is different from _eat_ and is before a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_substring_reverse_not("user/stuff/file.txtGARBAGEGARBAGEGARBAGE", "GARBAGE")
//   returns:                                      ^
//
constexpr s64 find_substring_reverse_not(const string &s, const string &eat, s64 start = 0) {
    assert(eat.Data && eat.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    auto *p   = utf8_get_cp_at_index_unsafe(s.Data, translate_index(start, s.Length, true) - 1);
    auto *end = s.Data + s.Count;

    auto *eatEnd = eat.Data + eat.Count;

    while (p > s.Data) {
        while (p != s.Data) {
            if (*p != *eat.Data) break;
            --p;
        }

        if (*p == *eat.Data && p == s.Data) return -1;

        auto *search   = p + 1;
        auto *progress = eat.Data + 1;
        while (search != end && progress != eatEnd && *search != *progress) ++search, ++progress;
        if (progress == eatEnd) return utf8_length(s.Data, p - s.Data);
        --p;
    }
    return -1;
}

// Searches for the last occurence of a code point which is different from _cp_ and is before a specified _start_ index.
// Returns -1 if no index was found.
//
//  e.g.
//   find_cp_reverse_not("user/stuff/file.txtCCCCCC", 'C')
//   returns:                               ^
//
constexpr s64 find_cp_reverse_not(const string &s, code_point cp, s64 start = 0) {
    utf8 encoded[4]{};
    utf8_encode_cp(encoded, cp);
    return find_substring_reverse_not(s, string(encoded, utf8_get_size_of_cp(encoded)), start);
}

// Searches for the first occurence of a code point which is also present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    start   = translate_index(start, s.Length);
    auto *p = utf8_get_cp_at_index_unsafe(s.Data, start);

    For(range(start, s.Length)) {
        if (find_cp(anyOfThese, utf8_decode_cp(p)) != -1) return utf8_length(s.Data, p - s.Data);
        p += utf8_get_size_of_cp(p);
    }
    return -1;
}

// Searches for the last occurence of a code point which is also present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_reverse_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    start   = translate_index(start, s.Length, true) - 1;
    auto *p = utf8_get_cp_at_index_unsafe(s.Data, start);

    For(range(start, -1, -1)) {
        if (find_cp(anyOfThese, utf8_decode_cp(p)) != -1) return utf8_length(s.Data, p - s.Data);
        p -= utf8_get_size_of_cp(p);
    }
    return -1;
}

// Searches for the first occurence of a code point which is NOT present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_not_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;

    start   = translate_index(start, s.Length);
    auto *p = utf8_get_cp_at_index_unsafe(s.Data, start);

    For(range(start, s.Length)) {
        if (find_cp(anyOfThese, utf8_decode_cp(p)) == -1) return utf8_length(s.Data, p - s.Data);
        p += utf8_get_size_of_cp(p);
    }
    return -1;
}

// Searches for the last occurence of a code point which is NOT present in _anyOfThese_ and is before a specified _start_ index.
// Returns -1 if no index was found.
constexpr s64 find_reverse_not_any_of(const string &s, const string &anyOfThese, s64 start = 0) {
    assert(anyOfThese.Data && anyOfThese.Length);

    if (s.Length == 0) return -1;

    if (start >= s.Length || start <= -s.Length) return -1;
    if (start == 0) start = s.Length;

    start   = translate_index(start, s.Length, true) - 1;
    auto *p = utf8_get_cp_at_index_unsafe(s.Data, start);

    For(range(start, -1, -1)) {
        if (find_cp(anyOfThese, utf8_decode_cp(p)) == -1) return utf8_length(s.Data, p - s.Data);
        p -= utf8_get_size_of_cp(p);
    }
    return -1;
}

// Counts the number of occurences of _cp_
constexpr s64 count(const string &s, code_point cp) {
    s64 result = 0, index = 0;
    while ((index = find_cp(s, cp, index)) != -1) {
        ++result, ++index;
        if (index >= s.Length) break;
    }
    return result;
}

// Counts the number of occurences of _str_
constexpr s64 count(const string &s, const string &str) {
    s64 result = 0, index = 0;
    while ((index = find_substring(s, str, index)) != -1) {
        ++result, ++index;
        if (index >= s.Length) break;
    }
    return result;
}

// Returns true if the string contains _cp_ anywhere
constexpr bool has(const string &s, code_point cp) { return find_cp(s, cp) != -1; }

// Returns true if the string contains _str_ anywhere
constexpr bool has(const string &s, const string &str) { return find_substring(s, str) != -1; }

//
// Substring and trimming:
//

// Gets [begin, end) range of characters into a new string object.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string substring(const string &s, s64 begin, s64 end) {
    if (begin == end) return "";

    s64 beginIndex = translate_index(begin, s.Length);
    s64 endIndex   = translate_index(end, s.Length, true);

    const char *beginPtr = utf8_get_cp_at_index_unsafe(s.Data, beginIndex);
    const char *endPtr   = beginPtr;
    For(range(beginIndex, endIndex)) endPtr += utf8_get_size_of_cp(endPtr);

    return string(beginPtr, endPtr - beginPtr);
}

// Returns true if _s_ begins with _str_
constexpr bool match_beginning(const string &s, const string &str) {
    if (str.Count > s.Count) return false;
    return compare_memory(s.Data, str.Data, str.Count) == -1;
}

// Returns true if _s_ ends with _str_
constexpr bool match_end(const string &s, const string &str) {
    if (str.Count > s.Count) return false;
    return compare_memory(s.Data + s.Count - str.Count, str.Data, str.Count) == -1;
}

// Returns a substring with white space removed at the start.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim_start(const string &s) { return substring(s, find_not_any_of(s, " \n\r\t\v\f"), s.Length); }

// Returns a substring with white space removed at the end.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim_end(const string &s) { return substring(s, 0, find_reverse_not_any_of(s, " \n\r\t\v\f") + 1); }

// Returns a substring with white space removed from both sides.
// This function doesn't allocate, but just returns a "view".
// We can do this because we don't store strings with a zero terminator.
constexpr string trim(const string &s) { return trim_end(trim_start(s)); }

//
// Operators:
//

constexpr bool operator==(const string &one, const string &other) { return compare_lexicographically(one, other) == 0; }
constexpr bool operator!=(const string &one, const string &other) { return !(one == other); }
constexpr bool operator<(const string &one, const string &other) { return compare_lexicographically(one, other) < 0; }
constexpr bool operator>(const string &one, const string &other) { return compare_lexicographically(one, other) > 0; }
constexpr bool operator<=(const string &one, const string &other) { return !(one > other); }
constexpr bool operator>=(const string &one, const string &other) { return !(one < other); }

// @TODO:
// constexpr auto operator<=>(const string &one, const string &other) { return compare_lexicographically(one, other); }

// operator<=> doesn't work on these for some reason...
constexpr bool operator==(const char *one, const string &other) { return compare_lexicographically(one, other) == 0; }
constexpr bool operator!=(const char *one, const string &other) { return !(one == other); }
constexpr bool operator<(const char *one, const string &other) { return compare_lexicographically(one, other) < 0; }
constexpr bool operator>(const char *one, const string &other) { return compare_lexicographically(one, other) > 0; }
constexpr bool operator<=(const char *one, const string &other) { return !(one > other); }
constexpr bool operator>=(const char *one, const string &other) { return !(one < other); }

// Substring operator:
// constexpr string string::operator()(s64 begin, s64 end) const { return substring(*this, begin, end); }

constexpr string string::operator[](substring_indices range) const { return substring(*this, range.b, range.e); }

// Be careful not to call this with _dest_ pointing to _src_!
// Returns just _dest_.
string *clone(string *dest, const string &src);

// Hash for strings
inline u64 get_hash(const string &value) {
    u64 hash        = 5381;
    For(value) hash = ((hash << 5) + hash) + it;
    return hash;
}

LSTD_END_NAMESPACE
