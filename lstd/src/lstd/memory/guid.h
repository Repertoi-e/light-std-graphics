#pragma once

#include "../common.h"

import lstd.string;

LSTD_BEGIN_NAMESPACE

// Used for generating unique ids
struct guid {
    stack_array<char, 16> Data;

    // By default the guid is zero
    constexpr guid() {
        const_zero_memory(&Data.Data[0], 16);
    }

    // Doesn't parse the string but just looks at the bytes.
    // See lstd.parse for parsing a string representation of a GUID.
    constexpr explicit guid(string data) {
        assert(data.Count == 16);
        const_copy_memory(&Data.Data[0], &data.Data[0], 16);
    }

    constexpr auto operator<=>(guid other) const { return Data <=> other.Data; }

    constexpr operator bool() { return guid{} != *this; }
};

// Hash for guid
inline u64 get_hash(guid value) {
    u64 hash             = 5381;
    For(value.Data) hash = (hash << 5) + hash + it;
    return hash;
}

// Guaranteed to generate a unique id each time (time-based)
guid guid_new();

LSTD_END_NAMESPACE
