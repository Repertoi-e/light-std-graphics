module;

#include "../common.h"

module lstd.memory;

import lstd.basic;
import lstd.path;
import lstd.fmt;
import lstd.os;

LSTD_BEGIN_NAMESPACE

// Copied from test.h
//
// We check if the path contains src/ and use the rest after that.
// Otherwise we just take the file name. Possible results are:
//
//      /home/.../game/src/some_dir/a/string.cpp ---> some_dir/a/localization.cpp
//      /home/.../game/some_dir/string.cpp       ---> localization.cpp
//
constexpr string get_short_file_name(string str) {
    char srcData[] = {'s', 'r', 'c', OS_PATH_SEPARATOR, '\0'};
    string src     = srcData;

    s64 findResult = string_find(str, src, -1, true);
    if (findResult == -1) {
        findResult = string_find(str, OS_PATH_SEPARATOR, -1, true);
        assert(findResult != string_length(str) - 1);
        // Skip the slash
        findResult++;
    } else {
        // Skip the src directory
        findResult += string_length(src);
    }

    string result = str;
    return substring(result, findResult, string_length(result));
}

#if defined DEBUG_MEMORY
debug_memory_node *new_node(allocation_header *header) {
    auto *node = (debug_memory_node *) pool_allocator(allocator_mode::ALLOCATE, &DebugMemoryNodesPool, sizeof(debug_memory_node), null, 0, 0);
    assert(node);

    zero_memory(node, sizeof(debug_memory_node));

    node->Header = header;

    // Leave invalid for now, filled out later
    node->ID = (u64) -1;

    return node;
}

void debug_memory_init() {
    AllocationCount = 0;

    DebugMemoryNodesPool.ElementSize = sizeof(debug_memory_node);

    s64 startingPoolSize = 5000 * sizeof(debug_memory_node) + sizeof(pool_allocator_data::block);

    void *pool = os_allocate_block(startingPoolSize);
    pool_allocator_provide_block(&DebugMemoryNodesPool, pool, startingPoolSize);

    // We allocate sentinels to simplify linked list management code
    auto sentinel1 = new_node((allocation_header *) 0);
    auto sentinel2 = new_node((allocation_header *) U64_MAX);

    sentinel1->Next = sentinel2;
    sentinel2->Prev = sentinel1;
    DebugMemoryHead = sentinel1;
    DebugMemoryTail = sentinel2;
}

void debug_memory_uninit() {
    if (Context.DebugMemoryPrintListOfUnfreedAllocationsAtThreadExitOrProgramTermination) {
        debug_memory_report_leaks();
    }

    auto *b = DebugMemoryNodesPool.Base;
    while (b) {
        auto *next = b->Next;
        os_free_block(b);
        b = next;
    }
}

file_scope auto *list_search(allocation_header *header) {
    debug_memory_node *t = DebugMemoryHead;
    while (t != DebugMemoryTail && t->Header < header) t = t->Next;
    return t;
}

file_scope debug_memory_node *list_add(allocation_header *header) {
    auto *n = list_search(header);
    assert(n->Header != header);

    auto *node = new_node(header);

    node->Next    = n;
    node->Prev    = n->Prev;
    n->Prev->Next = node;
    n->Prev       = node;

    return node;
}

file_scope debug_memory_node *list_remove(allocation_header *header) {
    auto *n = list_search(header);
    if (n->Header != header) return null;

    n->Prev->Next = n->Next;
    n->Next->Prev = n->Prev;

    return n;
}

bool debug_memory_list_contains(allocation_header *header) {
    return list_search(header)->Header == header;
}

void debug_memory_report_leaks() {
    debug_memory_maybe_verify_heap();

    s64 leaksCount = 0;

    // @Cleanup: Factor this into a macro
    auto *it = DebugMemoryHead->Next;
    while (it != DebugMemoryTail) {
        if (!it->Freed && !it->MarkedAsLeak) ++leaksCount;
        it = it->Next;
    }

    // @Cleanup @Platform @TODO @Memory Don't use the platform allocator. We should have a seperate allocator for debug info.
    auto **leaks = malloc<debug_memory_node *>({.Count = leaksCount, .Alloc = platform_get_persistent_allocator(), .Options = LEAK});
    defer(free(leaks));

    auto *p = leaks;

    it = DebugMemoryHead->Next;
    while (it != DebugMemoryTail) {
        if (!it->Freed && !it->MarkedAsLeak) *p++ = it;
        it = it->Next;
    }

    if (leaksCount) {
        print(">>> Warning: The module {!YELLOW}\"{}\"{!} terminated but it still had {!YELLOW}{}{!} allocations which were unfreed. Here they are:\n", os_get_current_module(), leaksCount);
    }

    For_as(i, range(leaksCount)) {
        auto *it = leaks[i];

        string file = "Unknown";

        //
        // @Cleanup D I R T Y @Cleanup @Cleanup @Cleanup
        //
        if (compare_string(it->AllocatedAt.File, "") != -1) {
            file = get_short_file_name(it->AllocatedAt.File);
        }

        print("    * {}:{} requested {!GRAY}{}{!} bytes, {{ID: {}, RID: {}}}\n", file, it->AllocatedAt.Line, it->Header->Size, it->ID, it->RID);
    }
}

file_scope void verify_node_integrity(debug_memory_node *node) {
    auto *header = node->Header;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // If an assert fires here it means that memory was messed up in some way.
    //
    // We check for several problems here:
    //   * No man's land was modified. This means that you wrote before or after the allocated block.
    //     This catches buffer underflows/overflows errors.
    //   * Alignment should not be 0, should be more than POINTER_SIZE (8 bytes) and should be a power of 2.
    //     If any of these is not true, then the header was definitely corrupted.
    //   * We store a pointer to the memory block at the end of the header, any valid header will have this
    //     pointer point after itself. Otherwise the header was definitely corrupted.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (node->Freed) {
        // We can't verify the memory was not modified because
        // it could be given back to the OS and to another program.

        return;
    }

    // The ID of the allocation to debug.
    auto id = node->ID;

    char noMansLand[NO_MANS_LAND_SIZE];
    fill_memory(noMansLand, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);

    auto *user = (char *) header + sizeof(allocation_header);
    assert(compare_memory((char *) user - NO_MANS_LAND_SIZE, noMansLand, NO_MANS_LAND_SIZE) == 0 &&
           "No man's land was modified. This means that you wrote before the allocated block.");

    assert(header->DEBUG_Pointer == user && "Debug pointer doesn't match. They should always match.");

    assert(compare_memory((char *) header->DEBUG_Pointer + header->Size, noMansLand, NO_MANS_LAND_SIZE) == 0 &&
           "No man's land was modified. This means that you wrote after the allocated block.");

    assert(header->Alignment && "Stored alignment is zero. Definitely corrupted.");
    assert(header->Alignment >= POINTER_SIZE && "Stored alignment smaller than pointer size (8 bytes). Definitely corrupted.");
    assert(is_pow_of_2(header->Alignment) && "Stored alignment not a power of 2. Definitely corrupted.");
}

void debug_memory_verify_heap() {
    auto *it = DebugMemoryHead->Next;
    while (it != DebugMemoryTail) {
        verify_node_integrity(it);
        it = it->Next;
    }
}

void debug_memory_maybe_verify_heap() {
    if (AllocationCount % Context.DebugMemoryHeapVerifyFrequency) return;
    debug_memory_verify_heap();
}

void check_for_overlapping_blocks(debug_memory_node *node) {
    // Check for overlapping memory blocks.
    // We can do this because we keep a sorted linked list of allocated blocks and we have info about their size.
    // This might catch bugs in allocator implementations or when two allocators share a pool.

    auto *left = node->Prev;
    while (left->Freed) left = left->Prev;

    auto *right = node->Next;
    while (right->Freed) right = right->Next;

    if (left != DebugMemoryHead) {
        // Check below
        s64 size = left->Header->Size + sizeof(allocation_header);
#if defined DEBUG_MEMORY
        size += NO_MANS_LAND_SIZE;
#endif
        if (((byte *) left->Header + size) > ((byte *) node->Header - node->Header->AlignmentPadding)) {
            assert(false && "Allocator implementation returned a pointer which overlaps with another allocated block (below). This can be due to a bug in the allocator code or because two allocators use the same pool.");
        }
    }

    if (right != DebugMemoryTail) {
        // Check above
        s64 size = node->Header->Size + sizeof(allocation_header);
#if defined DEBUG_MEMORY
        size += NO_MANS_LAND_SIZE;
#endif

        if (((byte *) node->Header + size) >= ((byte *) right->Header - right->Header->AlignmentPadding)) {
            assert(false && "Allocator implementation returned a pointer which overlaps with another allocated block (above). This can be due to a bug in the allocator code or because two allocators share the same pool.");
        }
    }
}
#endif

file_scope void *just_pad(void *p, s64 userSize, u32 align) {
    u32 padding          = calculate_padding_for_pointer_with_header(p, align, sizeof(allocation_header));
    u32 alignmentPadding = padding - sizeof(allocation_header);

    auto *result = (char *) p + alignmentPadding;
#if defined DEBUG_MEMORY
    fill_memory(result, CLEAN_LAND_FILL, userSize);
#endif
    return result;
}

file_scope void *encode_header(void *p, s64 userSize, u32 align, allocator alloc, u64 flags) {
    u32 padding          = calculate_padding_for_pointer_with_header(p, align, sizeof(allocation_header));
    u32 alignmentPadding = padding - sizeof(allocation_header);

    auto *result = (allocation_header *) ((char *) p + alignmentPadding);

    result->Alloc = alloc;
    result->Size  = userSize;

    result->Alignment        = align;
    result->AlignmentPadding = alignmentPadding;

    //
    // This is now safe since we handle alignment here (and not in general_(re)allocate).
    // Before I wrote the fix the program was crashing because of SIMD types,
    // which require memory to be 16 byte aligned. I tried allocating with specified
    // alignment but it wasn't taking into account the size of the allocation header
    // (accounting happened before bumping the resulting pointer here).
    //
    // Since I had to redo how alignment was handled I decided to remove ALLOCATE_ALIGNED
    // and REALLOCATE_ALIGNED and drastically simplify allocator implementations.
    // What we do now is request a block of memory with extra size
    // that takes into account possible padding for alignment.
    //                                                                              - 5.04.2020
    //
    // Now we do this differently (again) because there was a bug where reallocating was having
    // issues with _AlignmentPadding_. Now we require allocators to implement RESIZE instead of REALLOCATE
    // which mustn't move the block but instead return null if resizing failed to tell us we need to allocate a new one.
    // This moves handling reallocation entirely on our side, which, again is even cleaner.
    //                                                                              - 18.05.2020
    //
    p = result + 1;
    assert((((u64) p & ~((s64) align - 1)) == (u64) p) && "Pointer wasn't properly aligned.");

#if defined DEBUG_MEMORY
    fill_memory(p, CLEAN_LAND_FILL, userSize);

    fill_memory((char *) p - NO_MANS_LAND_SIZE, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);
    fill_memory((char *) p + userSize, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);

    result->DEBUG_Pointer = result + 1;
#endif

    return p;
}

file_scope void write_number(s64 num) {
    char number[20];

    auto *numberP  = number + 19;
    s64 numberSize = 0;
    {
        while (num) {
            *numberP-- = num % 10 + '0';
            num /= 10;
            ++numberSize;
        }
    }
    write(Context.Log, numberP + 1, numberSize);
}

// Without using the lstd.fmt module, i.e. without allocations.
file_scope void log_file_and_line(source_location loc) {
    write(Context.Log, loc.File);
    write(Context.Log, ":");
    write_number(loc.Line);
}

file_scope debug_memory_node *override_node_if_freed_or_add_a_new_one(allocation_header *header) {
    auto *node = list_search(header);
    if (node->Header == header) {
        if (!node->Freed) {
            // Maybe this is a bug in the allocator implementation,
            // or maybe two different allocators use the same pool.
            assert(false && "Allocator implementation returned a live pointer which wasn't freed yet.");
            return null;
        }

        node->Freed   = false;
        node->FreedAt = {};

        return node;
    } else {
        return list_add(header);
    }
}

void *general_allocate(allocator alloc, s64 userSize, u32 alignment, u64 options, source_location loc) {
    if (!alloc) alloc = Context.Alloc;
    assert(alloc && "Context allocator was null. The programmer should set it before calling allocate functions.");

    bool doHeader = true;

    // @Hack, have a way to specify this more robustly?
    if (alloc.Function == arena_allocator) doHeader = false;

    options |= Context.AllocOptions;

    if (alignment == 0) {
        auto contextAlignment = Context.AllocAlignment;
        assert(is_pow_of_2(contextAlignment));
        alignment = contextAlignment;
    }

#if defined DEBUG_MEMORY
    debug_memory_maybe_verify_heap();
    s64 id = AllocationCount;

    if (id == 650) {
        s32 k = 42;
    }
#endif

    alignment = alignment < POINTER_SIZE ? POINTER_SIZE : alignment;
    assert(is_pow_of_2(alignment));

    s64 required = userSize + alignment;
    if (doHeader) {
        required += sizeof(allocation_header) + sizeof(allocation_header) % alignment;

#if defined DEBUG_MEMORY
        required += NO_MANS_LAND_SIZE;  // This is for the safety bytes after the requested block
#endif
    } else {
        int a = 42;
    }

    void *block = alloc.Function(allocator_mode::ALLOCATE, alloc.Context, required, null, 0, options);
    assert(block);

    if (Context.LogAllAllocations && !Context._LoggingAnAllocation) {
        auto newContext                 = Context;
        newContext._LoggingAnAllocation = true;

        PUSH_CONTEXT(newContext) {
            write(Context.Log, ">>> Starting allocation at:   ");
            log_file_and_line(loc);
            // write(Context.Log, ", id: ");
            // write_number(id);
            write(Context.Log, ", block: ");
            write_number((s64) block);
            write(Context.Log, "\n");
        }
    }

    if (!doHeader) {
        // We don't do headers for certain allocators, e.g. the arena allocator (which is the type of the temporary allocator too).
        return just_pad(block, userSize, alignment);
    } else {
        auto *result = encode_header(block, userSize, alignment, alloc, options);

#if defined DEBUG_MEMORY
        // We need to do this subtraction instead of using _newBlock_,
        // because there might've been alignment padding.
        auto *header = (allocation_header *) result - 1;

        auto *node = override_node_if_freed_or_add_a_new_one(header);

        check_for_overlapping_blocks(node);

        node->ID = AllocationCount;
        atomic_inc(&AllocationCount);

        node->AllocatedAt = loc;

        node->RID          = 0;
        node->MarkedAsLeak = options & LEAK;
#endif
        return result;
    }
}

void *general_reallocate(void *ptr, s64 newUserSize, u64 options, source_location loc) {
    options |= Context.AllocOptions;

    auto *header = (allocation_header *) ptr - 1;

    void *block  = (byte *) header - header->AlignmentPadding;

    if (Context.LogAllAllocations && !Context._LoggingAnAllocation) [[unlikely]] {
        auto newContext                 = Context;
        newContext._LoggingAnAllocation = true;

        PUSH_CONTEXT(newContext) {
            write(Context.Log, ">>> Starting reallocation at: ");
            log_file_and_line(loc);
            // write(Context.Log, ", id: ");
            // write_number(node->ID);
            write(Context.Log, ", block: ");
            write_number((s64) block);
            write(Context.Log, "\n");
        }
    }

#if defined DEBUG_MEMORY
    debug_memory_maybe_verify_heap();

    auto *node = list_search(header);
    if (node->Header != header) {
        //
        // This may happen if you try to realloc a pointer allocated by another thread, a bumped pointer,
        // a pointer to a stack buffer, a pointer to memory allocated by allocators which
        // don't do headers (currently arena allocator -- temporary allocator).
        //
        // Reallocate only works on pointers on which we have encoded a header.
        //

        // @TODO: Callstack
        panic(tprint("{!RED}Attempting to reallocate a memory block which was not allocated in the heap.{!} This happened at {!YELLOW}{}:{}{!} (in function: {!YELLOW}{}{!}).", loc.File, loc.Line, loc.Function));
        return null;
    }

    if (node->Freed) {
        // @TODO: Callstack
        panic(tprint("{!RED}Attempting to reallocate a memory block which was freed.{!} The free happened at {!YELLOW}{}:{}{!} (in function: {!YELLOW}{}{!}).", node->FreedAt.File, node->FreedAt.Line, node->FreedAt.Function));
        return null;
    }

    auto id = node->ID;
#endif

    if (header->Size == newUserSize) [[unlikely]] {
        return ptr;
    }

    // The header stores just the size of the requested allocation
    // (so the user code can look at the header and not be confused with garbage)
    s64 extra = sizeof(allocation_header) + header->Alignment + sizeof(allocation_header) % header->Alignment;
#if defined DEBUG_MEMORY
    extra += NO_MANS_LAND_SIZE;
#endif

    s64 oldUserSize = header->Size;
    s64 oldSize     = oldUserSize + extra;
    s64 newSize     = newUserSize + extra;

    auto alloc = header->Alloc;

    void *result = ptr;

    // Try to resize the block, this returns null if the block can't be resized and we need to move it.
    void *newBlock = alloc.Function(allocator_mode::RESIZE, alloc.Context, newSize, block, oldSize, options);
    if (!newBlock) {
        // Memory needs to be moved
        void *newBlock = alloc.Function(allocator_mode::ALLOCATE, alloc.Context, newSize, null, 0, options);
        assert(newBlock);

        // We can't just override the header in the node, because we need
        // to keep the list sorted... so we need a new node.
        result = encode_header(newBlock, newUserSize, header->Alignment, alloc, options);

        // We need to do this subtraction instead of using _newBlock_,
        // because there might've been alignment padding.
        header = (allocation_header *) result - 1;

#if defined DEBUG_MEMORY
        // @Volatile
        auto rid             = node->RID;
        bool wasMarkedAsLeak = node->MarkedAsLeak;

        node = override_node_if_freed_or_add_a_new_one(header);

        // Copy old state
        node->ID           = id;
        node->RID          = rid;  // We increment this below
        node->MarkedAsLeak = wasMarkedAsLeak;

        // node->AllocatedAt = loc; // We set this below
#endif
        // Copy old data
        copy_memory_fast(result, ptr, oldUserSize);
        general_free(ptr, 0, loc);  // options?
    } else {
        //
        // The block was resized sucessfully and it doesn't need moving
        //

        assert(block == newBlock);  // Sanity

        header->Size = newUserSize;
    }

#if defined DEBUG_MEMORY
    check_for_overlapping_blocks(node);

    node->RID += 1;
    node->AllocatedAt = loc;

    if (oldSize < newSize) {
        // If we are expanding the memory, fill the new stuff with CLEAN_LAND_FILL
        fill_memory((char *) result + oldUserSize, CLEAN_LAND_FILL, newSize - oldSize);
    } else {
        // If we are shrinking the memory, fill the old stuff with DEAD_LAND_FILL
        fill_memory((char *) header + oldSize, DEAD_LAND_FILL, oldSize - newSize);
    }

    fill_memory((char *) result + newUserSize, NO_MANS_LAND_FILL, NO_MANS_LAND_SIZE);
#endif

    return result;
}

void general_free(void *ptr, u64 options, source_location loc) {
    if (!ptr) return;

    options |= Context.AllocOptions;

    auto *header = (allocation_header *) ptr - 1;

    void *block = (byte *) header - header->AlignmentPadding;

    if (Context.LogAllAllocations && !Context._LoggingAnAllocation) [[unlikely]] {
        auto newContext                 = Context;
        newContext._LoggingAnAllocation = true;

        PUSH_CONTEXT(newContext) {
            write(Context.Log, ">>> Starting free at:        ");
            log_file_and_line(loc);
            // write(Context.Log, ", id: ");
            // write_number(id);
            write(Context.Log, ", block: ");
            write_number((s64) block);
            write(Context.Log, "\n");
        }
    }

#if defined DEBUG_MEMORY
    debug_memory_maybe_verify_heap();

    auto *node = list_search(header);
    if (node->Header != header) {
        //
        // This may happen if you try to free a pointer allocated by another thread, a bumped pointer,
        // a pointer to a stack buffer, a pointer to memory allocated by allocators which
        // don't do headers (currently arena allocator -- temporary allocator).
        //
        // Free only works on pointers on which we have encoded a header.
        //

        // @TODO: Callstack
        panic(tprint("Attempting to free a memory block which was not heap allocated (in this thread)."));

        // Note: We don't support cross-thread freeing yet.

        return;
    }

    if (node->Freed) {
        panic(tprint("{!RED}Attempting to free a memory block which was already freed.{!} The previous free happened at {!YELLOW}{}:{}{!} (in function: {!YELLOW}{}{!})", node->FreedAt.File, node->FreedAt.Line, node->FreedAt.Function));
        return;
    }

    auto id = node->ID;
#endif

    auto alloc = header->Alloc;

    s64 extra = header->Alignment + sizeof(allocation_header) + sizeof(allocation_header) % header->Alignment;
#if defined DEBUG_MEMORY
    extra += NO_MANS_LAND_SIZE;
#endif

    s64 size = header->Size + extra;

#if defined DEBUG_MEMORY
    // If DEBUG_MEMORY we keep freed notes in the list
    // but mark them as freed. This allows debugging double freeing the same memory block.

    node->Freed   = true;
    node->FreedAt = loc;

    fill_memory(block, DEAD_LAND_FILL, size);
#endif

    alloc.Function(allocator_mode::FREE, alloc.Context, 0, block, size, options);
}

void free_all(allocator alloc, u64 options) {
#if defined DEBUG_MEMORY
    // Remove allocations made with the allocator from the the linked list so we don't corrupt the heap
    auto *it = DebugMemoryHead->Next;
    while (it != DebugMemoryTail) {
        if (!it->Freed) {
            if (it->Header->Alloc == alloc) {
                it->Freed   = true;
                it->FreedAt = source_location::current();
            }
        }
        it = it->Next;
    }
#endif

    options |= Context.AllocOptions;
    alloc.Function(allocator_mode::FREE_ALL, alloc.Context, 0, 0, 0, options);
}

LSTD_END_NAMESPACE

LSTD_USING_NAMESPACE;

extern "C" {

void *malloc(size_t size) { return (void *) malloc<byte>({.Count = (s64) size}); }

void *calloc(size_t num, size_t size) {
    void *block = malloc(num * size);
    zero_memory(block, num * size);
    return block;
}

void *realloc(void *block, size_t newSize) {
    if (!block) {
        return malloc(newSize);
    }
    return (void *) realloc((byte *) block, {.NewCount = (s64) newSize});
}

// No need to define this global function if the library was built without a namespace
void free(void *block) { free((byte *) block); }
}

[[nodiscard]] void *operator new(size_t size) { return general_allocate(Context.Alloc, size, 0, 0, source_location::current()); }
[[nodiscard]] void *operator new[](size_t size) { return general_allocate(Context.Alloc, size, 0, 0, source_location::current()); }

[[nodiscard]] void *operator new(size_t size, align_val_t alignment) { return general_allocate(Context.Alloc, size, (u32) alignment, 0, source_location::current()); }
[[nodiscard]] void *operator new[](size_t size, align_val_t alignment) { return general_allocate(Context.Alloc, size, (u32) alignment, 0, source_location::current()); }

void operator delete(void *ptr, align_val_t alignment) noexcept { general_free(ptr, 0, source_location::current()); }
void operator delete[](void *ptr, align_val_t alignment) noexcept { general_free(ptr, 0, source_location::current()); }
