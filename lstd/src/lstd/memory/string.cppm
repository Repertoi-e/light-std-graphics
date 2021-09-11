module;

#include "../common.h"
#include "../memory/delegate.h"

export module lstd.string;

export import lstd.memory;
export import lstd.array;
export import lstd.array_like;

LSTD_BEGIN_NAMESPACE

// @TODO: Make fully constexpr

//
// String doesn't guarantee a null termination at the end.
// It's essentially a data pointer and a count.
//
// This means that you can load a binary file into a string.
//
// The routines defined in array.cppm work with _string_ because
// _string_ is a typedef for array<char>. However they treat indices
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
    using string = array<char>;

    struct code_point_ref {
        string *Parent;
        s64 Index;

        code_point_ref(string *parent = null, s64 index = -1) : Parent(parent), Index(index) {}

        code_point_ref &operator=(code_point other);
        operator code_point() const;
    };

    constexpr s64 string_length(string s) { return utf8_length(s.Data, s.Count); }

    // The non-const version returns a special structure that allows assigning a new code point.
    // This is legal to do:
    //
    //      string a = "Hello";
    //      string_get(a, 0) = u8'Л';
    //      // a is now "Лello" and contains a two byte code point in the beginning.
    //
    // We need to do this because not all code points are 1 byte.
    //
    // We use a & here because taking a pointer is annoying for the caller. EH. MEH. What to do... what to doo..
    code_point_ref string_get(string & str, s64 index);

    // The const version just returns the decoded code point at that location
    constexpr code_point string_get(const string &str, s64 index);

    // Changes the code point at _index_ to a new one. May allocate and change the byte count of the string.
    void string_set(string * str, s64 index, code_point cp);

    // Doesn't allocate memory, strings in this library are not null-terminated.
    // We allow negative reversed indexing which begins at the end of the string,
    // so -1 is the last code point, -2 is the one before that, etc. (Python-style)
    constexpr string substring(string str, s64 begin, s64 end);

    //
    // Utilities to convert to c-style strings.
    // Functions for conversion between utf-8, utf-16 and utf-32 are provided in lstd.string_utils
    //

    // Allocates a buffer, copies the string's contents and also appends a zero terminator.
    // Uses the Context's current allocator. The caller is responsible for freeing.
    [[nodiscard("Leak")]] char *string_to_c_string(string s, allocator alloc = {});

    // Allocates a buffer, copies the string's contents and also appends a zero terminator.
    // Uses the temporary allocator.
    //
    // Implemented in lstd.context because otherwise we import in circle.
    char *string_to_c_string_temp(string s);

    //
    // String modification:
    //

    void string_insert_at_index(string * s, s64 index, const char *str, s64 size) {
        index = translate_index(index, string_length(*s));
        insert_at_index(s, index, str, size);
    }

    void string_insert_at_index(string * s, s64 index, string str) { string_insert_at_index(s, index, str.Data, str.Count); }

    void string_insert_at_index(string * s, s64 index, code_point cp) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, cp);
        string_insert_at_index(s, index, encodedCp, utf8_get_size_of_cp(cp));
    }

    void string_append(string * str, const char *ptr, s64 size) { insert_at_index(str, str->Count, ptr, size); }

    void string_append(string * str, string s) { insert_at_index(str, str->Count, s); }
    void string_append(string * str, code_point element) { string_insert_at_index(str, string_length(*str), element); }

    // Remove the first occurence of a code point.
    // Returns true on success (false if _cp_ was not found in the string).
    void string_remove(string * s, code_point cp) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, cp);

        s64 index = find(*s, string(encodedCp, utf8_get_size_of_cp(cp));
        if (index == -1) return false;

        remove_range(s, index, index + utf8_get_size_of_cp(cp));
    }

    // Remove code point at specified index.
    void string_remove_at_index(string * s, s64 index) {
        index = translate_index(index, string_length(*s));

        auto *t = utf8_get_pointer_to_cp_at_translated_index(s->Data, s->Count, index);

        s64 b = t - s.Data;
        remove_range(s, b, b + utf8_get_size_of_cp(t));
    }

    // Remove a range of code points. [begin, end)
    void string_remove_range(string * s, s64 begin, s64 end) {
        s64 length = string_length(*s);

        begin = translate_index(begin, length);
        end   = translate_index(end, length);

        auto *tb = utf8_get_pointer_to_cp_at_translated_index(s->Data, s->Count, begin);
        auto *te = utf8_get_pointer_to_cp_at_translated_index(s->Data, s->Count, end);
        remove_range(s, tb - s->Data, e - s->Data);
    }

    //
    // These get resolution in array.cppm.
    // For working with raw bytes and not code points...
    //
    // auto *insert_at_index(string *str, s64 index, char element);
    // auto *insert_at_index(string *str, s64 index, char *ptr, s64 size);
    // auto *insert_at_index(string *str, s64 index, array<char> arr2);
    // bool remove_ordered(string * str, char element);
    // bool remove_unordered(string * str, char element);
    // void remove_ordered_at_index(string * str, s64 index);
    // void remove_unordered_at_index(string * str, s64 index);
    // void remove_range(string * str, s64 begin, s64 end);
    // void replace_range(string * str, s64 begin, s64 end, string replace);
    //
    //
    // These actually work fine since we don't take indices, but we also provide
    // replace_all and remove_all overloads that work with code points and not single bytes:
    //
    // void replace_all(string * str, string search, string replace);          -- This works for substrings by default
    // void remove_all(string *str, string search);                            -- This as well
    //
    // void remove_all(string *str, char search);                                -- Remove all bytes
    // void replace_all(string * str, char search, char replace);                  -- Replace byte with byte
    // void replace_all(string * str, char search, string replace);              -- Replace byte with string
    // void replace_all(string *str, string search, char replace);               -- Replace string with byte

    void replace_all(string * s, code_point search, code_point replace) {
        char encodedOld[4];
        utf8_encode_cp(encodedOld, search);

        char encodedNew[4];
        utf8_encode_cp(encodedNew, replace);

        replace_all(s, string(encodedOld, utf8_get_size_of_cp(encodedOld)), string(encodedNew, utf8_get_size_of_cp(encodedNew)));
    }

    // Removes all occurences of _cp_
    void remove_all(string * s, code_point search) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, search);

        replace_all(s, string(encodedCp, utf8_get_size_of_cp(encodedCp)), string(""));
    }

    void remove_all(string * s, string search) { replace_all(s, search, string("")); }

    void replace_all(string * s, code_point search, string replace) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, search);

        replace_all(s, string(encodedCp, utf8_get_size_of_cp(encodedCp)), replace);
    }

    void replace_all(string * s, string search, code_point replace) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, replace);

        replace_all(s, search, string(encodedCp, utf8_get_size_of_cp(encodedCp)));
    }

    //
    // String searching:
    //

    //
    // Since strings are array_likes, these get resolution:
    //
    // constexpr s64 find(string str, const delegate<bool(char &)> &predicate, s64 start = 0, bool reversed = false);
    // constexpr s64 find(string str, char search, s64 start = 0, bool reversed = false);
    // constexpr s64 find(string str, string search, s64 start = 0, bool reversed = false);
    // constexpr s64 find_any_of(string str, string allowed, s64 start = 0, bool reversed = false);
    // constexpr s64 find_not(string str, char element, s64 start = 0, bool reversed = false);
    // constexpr s64 find_not_any_of(string str, string banned, s64 start = 0, bool reversed = false);
    // constexpr bool has(string str, char item);
    //
    // They work for bytes and take indices that point to bytes.
    //

    //
    // Here are versions that work with code points and with indices that point to code points.
    //

    constexpr s64 string_find(string str, const delegate<bool(code_point)> &predicate, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        s64 length = string_length(str);
        start      = translate_index(start, length);
        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) if (predicate(string_get(str, it))) return it;
        return -1;
    }

    constexpr s64 string_find(string str, code_point search, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        s64 length = string_length(str);
        start      = translate_index(start, length);
        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) if (string_get(str, it) == search) return it;
        return -1;
    }

    constexpr s64 string_find(string str, string search, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        if (!search.Data || search.Count == 0) return -1;

        s64 length = string_length(str);
        start      = translate_index(start, length);
        start      = translate_index(start, string_length(str));

        s64 searchLength = string_length(search);

        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) {
            s64 progress = 0;
            for (s64 s = it; progress != searchLength; ++s, ++progress) {
                if (!(string_get(str, s) == string_get(search, progress))) break;
            }
            if (progress == searchLength) return it;
        }
        return -1;
    }

    constexpr bool string_has(string str, code_point cp) {
        char encodedCp[4];
        utf8_encode_cp(encodedCp, cp);

        return string_find(str, string(encodedCp, utf8_get_size_of_cp(cp))) != -1;
    }

    constexpr s64 string_find_any_of(string str, string allowed, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        s64 length = string_length(str);
        start      = translate_index(start, length);
        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) if (string_has(allowed, string_get(str, it))) return it;
        return -1;
    }

    constexpr s64 string_find_not(string str, code_point search, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        s64 length = string_length(str);
        start      = translate_index(start, length);
        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) if (string_get(str, it) != search) return it;
        return -1;
    }

    constexpr s64 string_find_not_any_of(string str, string banned, s64 start = 0, bool reversed = false) {
        if (!str.Data || str.Count == 0) return -1;
        s64 length = string_length(str);
        start      = translate_index(start, length);
        For(range(start, reversed ? -1 : length, reversed ? -1 : 1)) if (!string_has(banned, string_get(str, it))) return it;
        return -1;
    }

    //
    // Comparison:
    //

    //
    // Since strings are array_likes, these get resolution:
    //
    // constexpr s64 compare(string arr1, string arr2);
    // constexpr s32 compare_lexicographically(string arr1, string arr2);
    //
    // However they return indices that point to bytes.
    //
    // Here we provide string comparison functions which return indices to code points.
    //
    // Note: To check equality (with operator == which is defined in array_like.cpp)
    // we check the bytes. However that doesn't always work for Unicode.
    // Some strings which have different representation might be considered equal.
    //    e.g. the character é can be represented either as 'é' or as '´' combined with 'e' (two characters).
    // Note that UTF-8 specifically requires the shortest-possible encoding for characters,
    // but you still have to be careful (some convertors might not necessarily do that).
    //
    //

    // Compares two utf-8 encoded strings and returns the index
    // of the code point at which they are different or _-1_ if they are the same.
    constexpr s64 string_compare(string s, string other) {
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

    // Compares two utf-8 encoded strings while ignoring case and returns the index
    // of the code point at which they are different or _-1_ if they are the same.
    constexpr s64 string_compare_ignore_case(string s, string other) {
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

    // Compares two utf-8 encoded strings lexicographically and returns:
    //  -1 if _a_ is before _b_
    //   0 if a == b
    //   1 if _b_ is before _a_
    constexpr s32 string_compare_lexicographically(string a, string b) {
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

    // Compares two utf-8 encoded strings lexicographically while ignoring case and returns:
    //  -1 if _a_ is before _b_
    //   0 if a == b
    //   1 if _b_ is before _a_
    constexpr s32 string_compare_lexicographically_ignore_case(string a, string b) {
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

    // Returns true if _s_ begins with _str_
    constexpr bool match_beginning(string s, string str) {
        if (str.Count > s.Count) return false;
        return compare_memory(s.Data, str.Data, str.Count) == -1;
    }

    // Returns true if _s_ ends with _str_
    constexpr bool match_end(string s, string str) {
        if (str.Count > s.Count) return false;
        return compare_memory(s.Data + s.Count - str.Count, str.Data, str.Count) == -1;
    }

    // Returns a substring with white space removed at the start
    constexpr string trim_start(string s) { return subarray(s, find_not_any_of(s, string(" \n\r\t\v\f")), s.Count); }

    // Returns a substring with white space removed at the end
    constexpr string trim_end(string s) { return subarray(s, 0, find_not_any_of(s, string(" \n\r\t\v\f"), s.Count, true) + 1); }

    // Returns a substring with white space removed from both sides
    constexpr string trim(string s) { return trim_end(trim_start(s)); }

    template <bool Const>
    struct string_iterator {
        using string_t = types::select_t<Const, const string, string>;

        string_t *Parent;
        s64 Index;

        string_iterator(string_t *parent = null, s64 index = 0) : Parent(parent), Index(index) {}

        string_iterator &operator++() {
            Index += 1;
            return *this;
        }

        string_iterator operator++(s32) {
            string_iterator temp = *this;
            ++(*this);
            return temp;
        }

        auto operator<=>(string_iterator other) const { return Index <=> other.Index; };
        auto operator*() { return string_get(*Parent, Index); }
        operator const char *() const { return utf8_get_pointer_to_cp_at_translated_index(Parent->Data, Parent->Count, Index); }
    };

    // To make range based for loops work.
    auto begin(string & str) { return string_iterator<false>(&str, 0); }
    auto end(string & str) { return string_iterator<false>(&str, string_length(str)); }

    auto begin(const string &str) { return string_iterator<true>(&str, 0); }
    auto end(const string &str) { return string_iterator<true>(&str, string_length(str)); }

    // Hashing strings...
    u64 get_hash(string value) {
        u64 hash        = 5381;
        For(value) hash = ((hash << 5) + hash) + it;
        return hash;
    }

    //
    // Implementation:
    //

    constexpr code_point string_get(const string &str, s64 index) {
        if (index < 0) {
            // @Speed... should we cache this in _string_?
            // We need to calculate the total length (in code points)
            // in order for the negative index to be converted properly.
            s64 length = string_length(str);
            index      = translate_index(index, length);

            // If LSTD_ARRAY_BOUNDS_CHECK is defined:
            // _utf8_get_pointer_to_cp_at_translated_index()_ also checks for out of bounds but we are sure the index is valid
            // (since translate_index also checks for out of bounds) so we can get away with the unsafe version here.
            auto *s = str.Data;
            For(range(index)) s += utf8_get_size_of_cp(s);
            return utf8_decode_cp(s);
        } else {
            auto *s = utf8_get_pointer_to_cp_at_translated_index(str.Data, str.Count, index);
            return utf8_decode_cp(s);
        }
    }

    constexpr string substring(string str, s64 begin, s64 end) {
        s64 length     = string_length(str);
        s64 beginIndex = translate_index(begin, length);
        s64 endIndex   = translate_index(end, length, true);

        const char *beginPtr = utf8_get_pointer_to_cp_at_translated_index(str.Data, str.Count, beginIndex);
        const char *endPtr   = beginPtr;

        // @Speed
        For(range(beginIndex, endIndex)) endPtr += utf8_get_size_of_cp(endPtr);

        return string((char *) beginPtr, (s64) (endPtr - beginPtr));
    }

    constexpr auto operator<=>(const char *one, string other) { return string_compare_lexicographically(string(one), other); }
}

LSTD_END_NAMESPACE
module : private;
LSTD_BEGIN_NAMESPACE

code_point_ref string_get(string &str, s64 index) { return code_point_ref(&str, index); }

code_point_ref &code_point_ref::operator=(code_point other) {
    string_set(Parent, Index, other);
    return *this;
}

code_point_ref::operator code_point() const {
    // Avoid infinite recursion by calling the right overload of string_get
    auto *constParent = (const string *) Parent;
    return string_get(*constParent, Index);
}

void string_set(string *str, s64 index, code_point cp) {
    const char *target = utf8_get_pointer_to_cp_at_translated_index(str->Data, str->Count, index);

    char encodedCp[4];
    utf8_encode_cp(encodedCp, cp);

    replace_range(str, target - str->Data, target - str->Data + utf8_get_size_of_cp(target), string(encodedCp, utf8_get_size_of_cp(cp)));
}

[[nodiscard("Leak")]] char *string_to_c_string(string s, allocator alloc) {
    char *result = malloc<char>({.Count = s.Count + 1, .Alloc = alloc});
    copy_memory(result, s.Data, s.Count);
    result[s.Count] = '\0';
    return result;
}

LSTD_END_NAMESPACE
