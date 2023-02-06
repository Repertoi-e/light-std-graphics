#pragma once

#include "namespace.h"
#include "platform.h"

LSTD_BEGIN_NAMESPACE

#if COMPILER == MSVC

//
// Defines some hooks so users of this library
// can add callbacks that run before/after main
// (like constructors and destructors of globals).
//

typedef void(__cdecl* lstd_callback)(void);

// God fucking bless Raymond Chen
// https://devblogs.microsoft.com/oldnewthing/20181107-00/?p=100155

#pragma section("LSTD_B$a", read)  
inline __declspec(allocate("LSTD_B$a")) lstd_callback g_LSTDFirstInit[] = { 0 };

#pragma section("LSTD_A$a", read)  
inline __declspec(allocate("LSTD_A$a")) lstd_callback g_LSTDFirstUninit[] = { 0 };

#pragma section("LSTD_B$b", read)
#pragma section("LSTD_B$c", read)
#pragma section("LSTD_A$b", read)
#pragma section("LSTD_A$c", read)

#define LSTD_ADD_EARLY_INITIALIZER(fn) __declspec(allocate("LSTD_B$b")) \
    lstd_callback initializer##fn = fn
#define LSTD_ADD_INITIALIZER(fn)       __declspec(allocate("LSTD_B$c")) \
    lstd_callback initializer##fn = fn

#define LSTD_ADD_EARLY_UNINITIALIZER(fn) __declspec(allocate("LSTD_A$b")) \
    lstd_callback uninitializer##fn = fn
#define LSTD_ADD_UNINITIALIZER(fn)       __declspec(allocate("LSTD_A$c")) \
    lstd_callback uninitializer##fn = fn

#pragma section("LSTD_B$z", read)  
inline __declspec(allocate("LSTD_B$z")) lstd_callback g_LSTDLastInit[] = { 0 };

#pragma section("LSTD_A$z", read)  
inline __declspec(allocate("LSTD_A$z")) lstd_callback g_LSTDLastUninit[] = { 0 };

#else
#error Implement. Compiler.
#endif

LSTD_END_NAMESPACE

