#include "lstd/common/common.h"

#if OS == WINDOWS

#include "lstd/common/windows.h"  // Declarations of Win32 functions
#include "lstd/io.h"
#include "lstd/memory/guid.h"

import path;
import fmt;
import os;

//
// This is here to assist cases where you want to share the memory between two modules (e.g. an exe and a dll or multiple dlls, etc.)
// By default, when you link lstd with the dll, each dll gets its own global state (global allocator, debug memory info, etc.),
// which means that allocations done in different modules are incompatible. If you provide a symbol lstd_dont_initialize_global_state_stub
// with the value "true" we don't initialize that global state (instead we leave it as null). That means that YOU MUST initialize it yourself!
// You must initialize the following global variables by passing the values from the "host" to the "guest" module:
//  - DEBUG_memory     (a global pointer, by default we allocate it)
//
// @Volatile: As we add more global state.
//
// Why do we do this?
// In another project (light-std-graphics) I have an exe which serves as the engine, and loads dlls (the game). We do this to support hot-loading
// so we can change the game code without closing the window. The game (dll) allocates memory and needs to do that from the engine's allocator and debug memory,
// otherwise problems occur when hot-loading a new dll.
//
// @Cleanup @Cleanup @Cleanup @Cleanup @Cleanup @Cleanup @Cleanup @Cleanup  There should be a better way and we should get rid of this.
//                                                                          I haven't thought much about it yet.
extern "C" bool lstd_init_global() { return true; }

// If the user didn't provide a definition for lstd_init_global, the linker shouldn't complain,
// but instead provide a stub function which returns true.
extern "C" bool lstd_init_global_stub() { return true; }
#pragma comment(linker, "/ALTERNATENAME:lstd_init_global=lstd_init_global_stub")

LSTD_BEGIN_NAMESPACE

void win64_crash_handler_init();

#undef LSTD_NO_CRT

// If we are building with NO CRT we call these functions in our entry point - main_no_crt.
// If we are linking with the CRT then we need to inject these callbacks, so the CRT calls them and initializes the state properly.
#if not defined LSTD_NO_CRT
// How it works is described in this awesome article:
// https://www.codeguru.com/cpp/misc/misc/applicationcontrol/article.php/c6945/Running-Code-Before-and-After-Main.htm#page-2
#if COMPILER == MSVC

void win64_monitor_init();
void win64_window_init();

file_scope s32 c_init() {
    // :PlatformStateInit
    internal::platform_init_context();
    internal::platform_init_global_state();
    win64_crash_handler_init();

    win64_monitor_init();
    win64_window_init();

    return 0;
}

file_scope s32 tls_init() {
    internal::platform_init_context();
    return 0;
}

void win64_monitor_uninit();
void win64_window_uninit();

file_scope s32 pre_termination() {
    // :PlatformExitTermination
    exit_call_scheduled_functions();

    win64_monitor_uninit();
    win64_window_uninit();

    internal::platform_uninit_state();
    return 0;
}

typedef s32 cb(void);
#pragma data_seg(".CRT$XIU")
static cb *lstd_cinit[] = {c_init};

#pragma data_seg(".CRT$XDU")
static cb *lstd_tlsinit[] = {tls_init};

#pragma data_seg(".CRT$XPU")
static cb *lstd_preterm[] = {pre_termination};

#else
#error @TODO: See how this works on other compilers!
#endif
#endif

guid guid_new() {
    GUID g;
    CoCreateGuid(&g);

    auto data = to_stack_array((byte) (g.Data1 >> 24 & 0xFF),
                               (byte) (g.Data1 >> 16 & 0xFF),
                               (byte) (g.Data1 >> 8 & 0xFF),
                               (byte) (g.Data1 & 0xff),

                               (byte) (g.Data2 >> 8 & 0xFF),
                               (byte) (g.Data2 & 0xff),

                               (byte) (g.Data3 >> 8 & 0xFF),
                               (byte) (g.Data3 & 0xFF),

                               g.Data4[0],
                               g.Data4[1],
                               g.Data4[2],
                               g.Data4[3],
                               g.Data4[4],
                               g.Data4[5],
                               g.Data4[6],
                               g.Data4[7]);
    return guid(data);
}

LSTD_END_NAMESPACE

#endif
