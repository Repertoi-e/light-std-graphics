module;

#include "../common.h"

export module lstd.guid;

import lstd.string;

LSTD_BEGIN_NAMESPACE

export {
    // Used for generating unique ids
    struct guid {
        stack_array<char, 16> Data;

        // By default the guid is zero
        constexpr guid() {
            For(range(16)) Data[it] = 0;
        }

        // Doesn't parse the string but just looks at the bytes.
        // See lstd.parse for parsing a string representation of a GUID.
        constexpr explicit guid(string data) {
            assert(data.Count == 16);
            For(range(16)) Data[it] = data.Data[it];
        }

        constexpr auto operator<=>(guid other) const { return Data <=> other.Data; }

        constexpr operator bool() {
            guid empty;
            return Data != empty.Data;
        }
    };

    // Guaranteed to generate a unique id each time (time-based)
    guid create_guid();

    // Hash for guid
    u64 get_hash(guid value) {
        u64 hash             = 5381;
        For(value.Data) hash = (hash << 5) + hash + it;
        return hash;
    }
}

LSTD_END_NAMESPACE
