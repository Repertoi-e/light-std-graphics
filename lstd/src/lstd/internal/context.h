#pragma once

#include "../memory/allocator.h"
#include "../thread.h"

LSTD_BEGIN_NAMESPACE

struct writer;

template <typename T>
struct array;

struct os_function_call;
struct string;

using panic_handler_t = void (*)(const string &message, const array<os_function_call> &callStack);
void default_panic_handler(const string &message, const array<os_function_call> &callStack);

using fmt_parse_error_handler_t = void (*)(const string &message, const string &formatString, s64 position);
void fmt_default_parse_error_handler(const string &message, const string &formatString, s64 position);

//
// Thread local global variable to control certain behaviours of the program.
// A way to store options that are valid in a certain scope or for certain threads.
//
// Gets initialized when the program runs for the main thread and tls_init (take a look at windows_common.cpp) initializes allocators.
// Options get copied to new threads (take a look at thread_wrapper in windows_thread.cpp).
//
struct context {
    thread::id ThreadID;  // The current thread's ID

    // :TemporaryAllocator: Take a look at the docs of this allocator in "allocator.h"
    // (or the allocator module if you are living in the future).
    //
    // This gets initialized the first time it gets used in a thread.
    // Each thread gets a unique temporary allocator to prevent data races and to remain fast.
    // Default size is 8 KiB but you can increase that by allocating a large block and then calling free_all.
    temporary_allocator_data TempAllocData;  // Initialized the first time it is used
    allocator Temp = {"no_init"};            // The "allocator object" for the temporary allocator with which allocations are made.

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Stuff that is unique to every thread.
    //
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Stuff that gets copied from the parent when a new thread is spawned.
    //
    // Layout is important because we copy everything after the _Temp_ member.
    //
    // @Platform
    // .. We do it that way because tls_init (windows_common.cpp) gets called automatically but it can't have enough info about
    // the parent context, but we still initialize the default allocator and the temporary allocator there.
    //
    // We do this in order to ensure threads get OUR allocator treatment in the maximum number of cases. Note that we are very invasive
    // in this regard but this allows the programmer to have control over allocations done in any library used by your project (even when
    // that library doesn't provide custom allocator callbacks).
    //
    // If we did everything in the thread wrapper (our thread API is the one that copies the context variables)
    // then threads that were not created with this library would not get the same allocator.
    //
    // *** Caveat: For libraries that call malloc/free directly (we override just new/delete) we can't force to use our allocator.
    // It might be possible to do linker hacks and provide a drop-out replacement version of malloc/free in the future, but for
    // now we DON'T do that. @TODO @Robustness @Platform
    //
    ///////////////////////////////////////////////////////////////////////////////////////

    //
    // This is a pair of a function and a data pointer.
    // Gets initialized to default (malloc) in *platform*_common.cpp
    // Change this (recommended way is to use the WITH_ALLOC macro) in order to
    // change the allocator which a piece of code uses transparently.
    //
    // This makes it so when you call a function, the caller doesn't have to pass a parameter,
    // the function can just allocate normally, without knowing that the caller has changed the allocator.
    //
    // When allocating you should use the context's allocator (allocate<> and allocate_array<> does
    // that by default - unless overriden explicitly, we also override operator new and delete).
    //
    // The idea for this comes from the implicit context in Jai.
    allocator Alloc;  // = DefaultAlloc by default. Initialized in windows_common.cpp. @Platform

    u16 AllocAlignment;  // By default == POINTER_SIZE (8)

    // Any options that get OR'd with the options in any allocation (options are implemented as flags).
    // e.g. using the LEAK flag, you can mark the allocation as a leak (doesn't get reported when calling DEBUG_memory->report_leaks()).
    u64 AllocOptions;

    // Used for debugging. Every time an allocation is made, logs info about it.
    bool LogAllAllocations;
    bool LoggingAnAllocation;  // Don't set. Used to avoid infinite looping when the above bool is true.

    // Gets called when the program encounters an unhandled expection.
    // This can be used to view the stack trace before the program terminates.
    // The default handler prints the crash message and stack trace to _Log_.
    panic_handler_t PanicHandler;
    bool HandlingPanic;  // Don't set. Used to avoid infinite looping when handling panics. Don't touch!

    // When printing you should use this variable.
    // This makes it so users can redirect logging output.
    // By default it points to cout (the console).
    writer *Log;

    // By default when we encounter an invalid format string we panic the program.
    // One might want to silence such errors and just continue executing, or redirect the error - like we do in the tests.
    fmt_parse_error_handler_t FmtParseErrorHandler;

    // Disable stylized text output (colors, background colors, and bold/italic/strikethrough/underline text).
    // This is useful when logging to files/strings and not the console. The ansi escape codes look like garbage in files/strings.
    bool FmtDisableAnsiCodes;

    context() {} // Doesn't do anything. That's important.
};

// Immutable context available everywhere. Contains certain variables that are "global" to the program,
// but you may change them cleanly from scope to scope. e.g. You can turn on a certain allocator for part
// of the program without that code knowing it's using a different allocator - because allocations (*by default)
// are using the Context allocator. Another use case:
//
// The current state gets copied from parent thread to the new thread when creating a thread.
//
// Modify this with the macros WITH_CONTEXT_VAR, WITH_ALLOC, ... etc,
// they restore the old value at the end of the scope that immediately follows them.
//
// Note that you can modify this "globally" by using a cast and circumventing the C++ type system,
// but please don't do that :D. The reason this is a const variable is that it enforces the programmer
// to not do things that aren't meant to be done (imagine a library just blindly setting a Context variable
// inside a function and not restoring it at the end, now your whole program is using that new variable
// even though you may not want that ... the author of the library may still be able to do that maliciously
// with a const cast but he can also do 1000 different things that completely break your program so...
// at least this way we are sure it's not a bug).
inline const thread_local context Context;

// This is a helper macro to safely modify a variable in the implicit context in a block of code.
// Usage:
//    WITH_CONTEXT_VAR(variable, newVariableValue) {
//        ... code with new context variable ...
//    }
//    ... old context variable value is restored ...
//
// @Constcast
#define WITH_CONTEXT_VAR(var, newValue)                          \
    auto LINE_NAME(oldVar) = Context.var;                        \
    auto LINE_NAME(restored) = false;                            \
    defer({                                                      \
        if (!LINE_NAME(restored)) {                              \
            ((context *) &Context)->var = LINE_NAME(oldVar);     \
        }                                                        \
    });                                                          \
    if (true) {                                                  \
        ((context *) &Context)->var = newValue;                  \
        goto LINE_NAME(body);                                    \
    } else                                                       \
        while (true)                                             \
            if (true) {                                          \
                ((context *) &Context)->var = LINE_NAME(oldVar); \
                LINE_NAME(restored) = true;                      \
                break;                                           \
            } else                                               \
                LINE_NAME(body) :

// Shortcut for allocators
#define WITH_ALLOC(newAlloc) WITH_CONTEXT_VAR(Alloc, newAlloc)

//
// These were moved from allocator.h where they made sense to be, but we need to access Context.Alloc here.
// In the future we hopefully find a way to structure the library so these problems are avoided.
// We can't make them non-templates because we need the type info...
//

template <typename T>
concept non_void = !types::is_same<T, void>;

LSTD_END_NAMESPACE

#if BITS == 64
using size_t = u64;
#else
using size_t = u32;
#endif
using align_val_t = size_t;

// :AvoidSTDs:
// Normally <new> defines the placement new operator but if we avoid using headers from the C++ STD we define our own implementation here.
// Note: You must tell us with a macro: LSTD_DONT_DEFINE_STD.
//
// By default we avoid STDs (like in real life) but if e.g. a library relies on it we would get definition errors.
// In general this library can work WITH or WITHOUT the normal standard library.
#if defined LSTD_DONT_DEFINE_STD
#include <new>
#else
#if COMPILER == MSVC
// Note: If you get many compile errors (but you have defined LSTD_DONT_DEFINE_STD).
// You probably need to define it globally, because not all headers from this library see the macro.
inline void *__cdecl operator new(size_t, void *p) noexcept { return p; }
inline void *__cdecl operator new[](size_t, void *p) noexcept { return p; }
#else
inline void *operator new(size_t, void *p) noexcept { return p; }
inline void *operator new[](size_t, void *p) noexcept { return p; }
#endif
#endif

LSTD_BEGIN_NAMESPACE

template <non_void T>
T *lstd_allocate_impl(s64 count, allocator alloc, u32 alignment, u64 options, source_location loc) {
    s64 size = count * sizeof(T);

    if (!alloc) alloc = Context.Alloc;
    auto *result = (T *) general_allocate(alloc, size, alignment, options, loc);

    if constexpr (!types::is_scalar<T>) {
        auto *p = result;
        auto *end = result + count;
        while (p != end) {
            new (p) T;
            ++p;
        }
    }
    return result;
}

// Note: We don't support "non-trivially copyable" types (types that can have logic in the copy constructor).
// We assume your type can be copied to another place in memory and just work.
// We assume that the destructor of the old copy doesn't invalidate the new copy.
template <non_void T>
requires(!types::is_const<T>) T *lstd_reallocate_array_impl(T *block, s64 newCount, u64 options, source_location loc) {
    if (!block) return null;

    // I think the standard implementation frees in this case but we need to decide
    // what _options_ should go there (no options or the ones passed to reallocate?),
    // so we leave that up to the call site.
    assert(newCount != 0);

    auto *header = (allocation_header *) block - 1;
    s64 oldCount = header->Size / sizeof(T);

    if constexpr (!types::is_scalar<T>) {
        if (newCount < oldCount) {
            auto *p = block + newCount;
            auto *end = block + oldCount;
            while (p != end) {
                p->~T();
                ++p;
            }
        }
    }

    s64 newSize = newCount * sizeof(T);
    auto *result = (T *) general_reallocate(block, newSize, options, loc);

    if constexpr (!types::is_scalar<T>) {
        if (oldCount < newCount) {
            auto *p = result + oldCount;
            auto *end = result + newCount;
            while (p != end) {
                new (p) T;
                ++p;
            }
        }
    }
    return result;
}

//
// More philosophy on this library...
//
// We don't use new and delete.
// 1) The syntax is ugly in my opinion.
// 2) You have to be careful not to mix "new" with "delete[]"/"new[]" and "delete".
// 3) It's an operator and can be overriden by structs/classes.
// 4) Modern C++ people say not to use new/delete as well, so ..
//
// Now seriously, there are two ways to allocate memory in C++: malloc and new.
// Both are far from optimal, so let's introduce another way!
// Note: We override the default operator new/delete to call our version.
//       When we don't link with the CRT, malloc is undefined, we provide a replacement.
//       When we link with the CRT, we do it dynamically, so we can redirect calls to malloc to our replacement.
//
// The following functions are defined:
//  allocate,
//  allocate_array,
//  reallocate_array
//  free
//
// All work as expected.
//
// Note: allocate and allocate_array call constructors on non-scalar values, free calls destructors (make sure you pass the right pointer type to free!)
//
// We allocate a bit of space (28 bytes without debugging info) before the block to store a header with information (the size of the allocation, the alignment,
// the allocator with which it was allocated, and debugging info if DEBUG_MEMORY is defined - see comments in allocator.h).
//
// There are some assumptions we make:
//   Your types are "trivially copyable" which means that they can be copied byte by byte to another place and still work.
//   That means that we don't call copy/move constructors when reallocating.
//
// In general our array<> type also expects type to be simple data. If you need extra logic when copying memory, implement that explicitly (not with a copy constructor).
// We do that to avoid C++ complicated shit that drives sane people insane.
// Here we don't do C++ style exceptions, we don't do copy/move constructors, we don't do destructors, we don't do any of that.
//
// Note: I said we don't do destructors but we still call them. That's because sometimes they are useful and can really simplify the code.
//   However since arrays and even the basic allocation functions copy byte by byte, that means that types that own memory (like string) must
//   be implemented differently - which means that destructors become useless (take a look at the type policy in context.h).
//   The recommended way to implement "destructors" is to override the free() function with your type as a reference parameter.
//
// e.g
//   string a;
//   free(a); // Not free(&a)!
//
//
// The functions also allow certain options to be specified. Using C++20 syntax, here are a few examples on call that looks:
//
// auto *node = allocate<ast_binop>();
// auto *node = allocate<ast_binop>({.Alloc = Context.Temp});
// auto *node = allocate<ast_binop>({.Options = LEAK});
//
// auto *simdType = allocate<f32v4>({.Alignment = 16});
//
// auto *memory = allocate_array<byte>(200);
// auto *memory = allocate_array<byte>(200, {.Alloc = Context.Temp, .Alignment = 64, .Options = LEAK});
//
//
// The functions take source_location as a final parameter. That is set automatically to current() which means the caller site.
// This info is saved to the header when DEBUG_MEMORY is defined.
//

struct allocate_options {
    allocator Alloc = {};
    u32 Alignment = 0;
    u64 Options = 0;
};

// T is used to initialize the resulting memory (uses placement new to call the constructor).
template <typename T>
T *allocate(allocate_options options = {}, source_location loc = source_location::current()) {
    return lstd_allocate_impl<T>(1, options.Alloc, options.Alignment, options.Options, loc);
}

// T is used to initialize the resulting memory (uses placement new to call the constructor).
template <typename T>
T *allocate_array(s64 count, allocate_options options = {}, source_location loc = source_location::current()) {
    return lstd_allocate_impl<T>(count, options.Alloc, options.Alignment, options.Options, loc);
}

// Note: We don't support "non-trivially copyable" types (types that can have logic in the copy constructor).
// We assume your type can be copied to another place in memory and just work.
// We assume that the destructor of the old copy doesn't invalidate the new copy.
template <typename T>
T *reallocate_array(T *block, s64 newCount, s64 reallocateOptions = 0, source_location loc = source_location::current()) {
    return lstd_reallocate_array_impl<T>(block, newCount, reallocateOptions, loc);
}

// If T is non-scalar we call the destructors on the objects in the memory block (determined by T, so make sure you pass a correct pointer type)
template <typename T>
requires(!types::is_const<T>) void free(T *block, u64 options = 0) {
    if (!block) return;

    s64 sizeT = 1;
    if constexpr (!types::is_same<T, void>) {
        sizeT = sizeof(T);
    }

    auto *header = (allocation_header *) block - 1;
    s64 count = header->Size / sizeT;

    if constexpr (!types::is_same<T, void> && !types::is_scalar<T>) {
        auto *p = block;
        while (count--) {
            p->~T();
            ++p;
        }
    }

    general_free(block, options);
}

LSTD_END_NAMESPACE

#if not defined LSTD_DONT_DEFINE_STD
[[nodiscard]] void *operator new(size_t size);
[[nodiscard]] void *operator new[](size_t size);

[[nodiscard]] void *operator new(size_t size, align_val_t alignment);
[[nodiscard]] void *operator new[](size_t size, align_val_t alignment);
#endif

void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;

void operator delete(void *ptr, align_val_t alignment) noexcept;
void operator delete[](void *ptr, align_val_t alignment) noexcept;
