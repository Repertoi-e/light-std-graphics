module;

#include "../common.h"

export module lstd.string_builder;

export import lstd.memory;
export import lstd.string;

LSTD_BEGIN_NAMESPACE

// Good for building large strings because it doesn't have to constantly reallocate.
// Starts with a 1_KiB buffer on the stack, if that fills up, allocates on the heap using _Alloc_.
// We provide an explicit allocator so you can set it in the beginning, before it ever allocates.
// If it's still null when we require a new buffer we use the Context's one.
export {
    struct string_builder {
        static constexpr s64 BUFFER_SIZE = 1_KiB;

        struct buffer {
            u8 Data[BUFFER_SIZE]{};
            s64 Occupied = 0;
            buffer *Next = null;
        };

        buffer BaseBuffer;
        buffer *CurrentBuffer = null;  // null means BaseBuffer. We don't point to BaseBuffer because pointers to other members are dangerous when copying.

        // Counts how many buffers have been dynamically allocated.
        s64 IndirectionCount = 0;

        // The allocator used for allocating new buffers past the first one (which is stack allocated).
        // This value is null until this object allocates memory (in which case it sets
        // it to the Context's allocator) or the user sets it manually.
        allocator Alloc;

        string_builder() {}
    };

    // Don't free the buffers, just reset cursor
    void reset(string_builder * builder);

    // Free any memory allocated by this object and reset cursor
    void free_buffers(string_builder * builder);

    // Append a code point to the builder
    void append(string_builder * builder, code_point codePoint);

    // Append a string to the builder
    void append(string_builder * builder, string str);

    // Append _size_ bytes from _data_ to the builder
    void append(string_builder * builder, const char *data, s64 size);

    // Merges all buffers in one string.
    // Maybe release the buffers as well?
    // The most common use case is builder_to_string() and then free_buffers() -- the builder is not needed anymore.
    [[nodiscard("Leak")]] string builder_to_string(string_builder * builder);
}

string_builder::buffer *get_current_buffer(string_builder &builder);

LSTD_END_NAMESPACE
