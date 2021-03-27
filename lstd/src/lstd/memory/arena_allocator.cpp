#include "../internal/context.h"
#include "allocator.h"

LSTD_BEGIN_NAMESPACE

#if COMPILER == MSVC
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

void *arena_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 options) {
    auto *data = (arena_allocator_data *) context;

    switch (mode) {
        case allocator_mode::ADD_POOL: {
            auto *pool = (allocator_pool *) oldMemory;  // _oldMemory_ is the parameter which should contain the block to be added
                                                        // the _size_ parameter contains the size of the block

            if (!allocator_pool_initialize(pool, size)) return null;
            allocator_pool_add_to_linked_list(&data->Base, pool);
            return pool;  // Returns null on failure, the added pool otherwise
        }
        case allocator_mode::REMOVE_POOL: {
            auto *pool = (allocator_pool *) oldMemory;

            void *result = allocator_pool_remove_from_linked_list(&data->Base, pool);
            return result;  // Returns null on failure, the removed pool otherwise
        }
        case allocator_mode::ALLOCATE: {
            auto *p = data->Base;
            while (p->Next) {
                if (p->Used + size < p->Size) break;
                p = p->Next;
            }

            if (p->Used + size >= p->Size) return null;  // Not enough space

            void *usableBlock = p + 1;
            void *result = (byte *) usableBlock + p->Used;

            p->Used += size;
            data->TotalUsed += size;

            return result;
        }
        case allocator_mode::RESIZE: {
            // Implementing a fast RESIZE requires finding in which block the memory is in.
            // We might store a header which tells us that but right now I don't think it's worth it.
            // We simply return null and let the reallocate function allocate a new block and copy the contents.
            //
            // If you are dealing with very very large blocks and copying is expensive, you should
            // implement a specialized allocator. If you are dealing with appending to strings
            // (which causes string to try to reallocate), we provide a string_builder utility which will help with that.
            return null;
        }
        case allocator_mode::FREE: {
            // We don't free individual allocations in the arena allocator

            // null means success FREE
            return null;
        }
        case allocator_mode::FREE_ALL: {
            auto *p = data->Base;
            while (p) {
                p->Used = 0;
                p = p->Next;
            }

            data->TotalUsed = 0;

            // null means successful FREE_ALL
            // (void *) -1 means that the allocator doesn't support FREE_ALL (by design)
            return null;
        }
        default:
            assert(false);
    }
    return null;
}

#if COMPILER == MSVC
#pragma warning(pop)
#endif

LSTD_END_NAMESPACE
