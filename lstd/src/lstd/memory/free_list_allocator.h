#pragma once

#include "allocator.h"

LSTD_BEGIN_NAMESPACE

struct free_list_allocator_data : non_copyable, non_movable, non_assignable {
    struct block {
        void *Address;
        block *Next;
        s64 Size;
    };

    struct heap {
        block *Free;   // First free block
        block *Used;   // First used block
        block *Fresh;  // First available blank block
        u64 Top;       // Top free address

        s16 MaxBlocks;
        s16 SplitThresh;

        const void *Limit = null;  // The end of the given block
    };

    // We store the heap in the beginning of the given base address, to be more cache-friendly
    // (although large blocks usually don't fit in the cache anyway).. this doesn't hurt.
    heap *Heap;

    // Expects a pre-allocated block.
    // See documentation at the bottom of this file.
    void init(const void *base, s64 size, s16 maxBlocks = 256, s16 splitThresh = 16);

    void *allocate_block(s64 num);
    void *resize_block(void *block, s64 newSize); // @TODO: Not implemented yet.
    void free_block(void *ptr);

    s64 count_free();   // Counts free blocks
    s64 count_used();   // Counts used blocks
    s64 count_fresh();  // Counts fresh blocks - fresh blocks are block that have been used. If we are out of free blocks, we start using those.
};

//
// This allocator uses a pre-allocated memory block and book keeps 3 linked lists: fresh blocks, used blocks, free blocks.
//
// It's suited for general use. A couple of parameters define behaviour. 
// _maxBlocks_ says into how many free blocks does the initial memory block get split into.
// _splitThresh_ determines the minimum size of a free block created by splitting. 
//               When allocating a block, if the free block is much larger than the requested size (specifically _splitThresh_ bytes larger) 
//               we create another free block out of the remaining space.
// 
// After allocation/freeing consecutive free blocks are merged together, which reduces fragmentation.
//
// You can get statistics about the allocator using the count_*() functions.
void *free_list_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *);

LSTD_END_NAMESPACE
