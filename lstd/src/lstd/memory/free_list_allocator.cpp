#include "free_list_allocator.h"

LSTD_BEGIN_NAMESPACE

void free_list_allocator_data::init(const void *base, s64 size, s16 maxBlocks, s16 splitThresh) {
    Heap = (heap *) base;
    Heap->Limit = (byte *) base + size;
    Heap->SplitThresh = splitThresh;
    Heap->MaxBlocks = maxBlocks;

    Heap->Free = null;
    Heap->Used = null;
    Heap->Fresh = (block *) Heap + 1;
    Heap->Top = (s64)(Heap->Fresh + maxBlocks);

    block *b = Heap->Fresh;
    For(range(maxBlocks)) {
        b->Next = b + 1;
        ++b;
    }
    b->Next = null;
}

void insert_block(free_list_allocator_data::block **free, free_list_allocator_data::block *b);
void compact(free_list_allocator_data::block **fresh, free_list_allocator_data::block *free);

void *free_list_allocator_data::allocate_block(s64 size) {
    block *ptr = Heap->Free;
    block *prev = null;

    u64 top = Heap->Top;
    while (ptr) {
        bool isTop = ((u64) ptr->Address + ptr->Size >= top) && ((u64) ptr->Address + size <= (u64) Heap->Limit);
        if (isTop || ptr->Size >= size) {
            if (prev) {
                prev->Next = ptr->Next;
            } else {
                Heap->Free = ptr->Next;
            }

            ptr->Next = Heap->Used;
            Heap->Used = ptr;
            if (isTop) {
                // print_s("resize top block");
                ptr->Size = size;
                Heap->Top = (size_t) ptr->Address + size;
            } else if (Heap->Fresh) {
                size_t excess = ptr->Size - size;
                if (excess >= Heap->SplitThresh) {
                    ptr->Size = size;

                    block *split = Heap->Fresh;
                    Heap->Fresh = split->Next;
                    split->Address = (void *) ((u64) ptr->Address + size);
                    // print_s("split");
                    // print_i((size_t) split->addr);
                    split->Size = excess;
                    insert_block(&Heap->Free, split);
                    compact(&Heap->Fresh, Heap->Free);
                }
            }
            return ptr->Address;
        }
        prev = ptr;
        ptr = ptr->Next;
    }

    // No matching free blocks, see if any other blocks available
    size_t newTop = top + size;
    if (Heap->Free != null && newTop <= (u64) Heap->Limit) {
        ptr = Heap->Fresh;
        Heap->Fresh = ptr->Next;
        ptr->Address = (void *) top;
        ptr->Next = Heap->Used;
        ptr->Size = size;
        Heap->Used = ptr;
        Heap->Top = newTop;
        return ptr->Address;
    }

    return null;
}

void free_list_allocator_data::free_block(void *ptr) {
    block *b = Heap->Used;
    block *prev = NULL;
    while (b) {
        if (ptr == b->Address) {
            if (prev) {
                prev->Next = b->Next;
            } else {
                Heap->Used = b->Next;
            }
            insert_block(&Heap->Free, b);
            compact(&Heap->Fresh, Heap->Free);
            return;
        }
        prev = b;
        b = b->Next;
    }

#if defined DEBUG_MEMORY
    // Sanity
    assert(count_free() + count_used() + count_fresh() == Heap->MaxBlocks);
#endif
}

void *free_list_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *) {
    auto *data = (free_list_allocator_data *) context;

    switch (mode) {
        case allocator_mode::ALLOCATE: {
            return data->allocate_block(size);
        }
        case allocator_mode::RESIZE: {
            // return data->resize_block(oldMemory, size);
            return null;
        }
        case allocator_mode::FREE: {
            data->free_block(oldMemory);
            return null;
        }
        case allocator_mode::FREE_ALL: {
            data->init(data->Heap, (s64) ((byte *) data->Heap->Limit - (byte *) data->Heap), data->Heap->MaxBlocks, data->Heap->SplitThresh);
            return null;
        }
        default:
            assert(false);
    }
    return null;
}

using block = free_list_allocator_data::block;

file_scope s64 count_blocks(block *ptr) {
    s64 num = 0;
    while (ptr != NULL) {
        num++;
        ptr = ptr->Next;
    }
    return num;
}

s64 free_list_allocator_data::count_free() { return count_blocks(Heap->Free); }
s64 free_list_allocator_data::count_used() { return count_blocks(Heap->Used); }
s64 free_list_allocator_data::count_fresh() { return count_blocks(Heap->Fresh); }

file_scope void insert_block(block **free, block *b) {
    block *ptr = *free;
    block *prev = null;
    while (ptr) {
        if ((u64) b->Address <= (u64) ptr->Address) break;
        prev = ptr;
        ptr = ptr->Next;
    }

    if (prev) {
        prev->Next = b;
    } else {
        (*free) = b;
    }

    b->Next = ptr;
}

file_scope void release_blocks(block **fresh, block *scan, block *to) {
    block *next;
    while (scan != to) {
        // print_s("release");
        // print_i((size_t) scan);
        next = scan->Next;
        scan->Next = (*fresh);
        *fresh = scan;
        scan->Address = 0;
        scan->Size = 0;
        scan = next;
    }
}

file_scope void compact(block **fresh, block *free) {
    block *prev;
    block *scan;

    block *ptr = free;
    while (ptr) {
        prev = ptr;
        scan = ptr->Next;
        while (scan && (u64) prev->Address + prev->Size == (s64) scan->Address) {
            // print_s("merge");
            // print_i((size_t) scan);
            prev = scan;
            scan = scan->Next;
        }

        if (prev) {
            size_t new_size = (u64) prev->Address - (u64) ptr->Address + prev->Size;
            // print_s("new size");
            // print_i(new_size);
            ptr->Size = new_size;
            block *next = prev->Next;

            // Make merged blocks available
            release_blocks(fresh, ptr->Next, prev->Next);

            // Relink
            ptr->Next = next;
        }
        ptr = ptr->Next;
    }
}

LSTD_END_NAMESPACE
