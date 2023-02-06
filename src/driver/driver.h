#pragma once

import "lstd.h";
import lstd.fmt;

#include <lstd-graphics/graphics.h>

LSTD_USING_NAMESPACE;  // The library is in it's own namespace for the sake of not being too invasive. I don't really want a namespace though!

// DRIVER_API is used to export functions from the game dll
#if OS == WINDOWS
#if !defined BUILDING_DRIVER
#define DRIVER_API extern "C" __declspec(dllexport)
#else
#define DRIVER_API __declspec(dllimport)
#endif
#else
#pragma warning "Unknown dynamic link import / export semantics."
#endif

// 'x' needs to have dll-interface to be used by clients of struct 'y'
// This will never be a problem since nowhere do we change struct sizes based on debug/release/whatever conditions
#if COMPILER == MSVC
#pragma warning(disable : 4251)
#endif

// All window related calls need to happen in the exe because of internal state kept by lstd.
// The .dll gets it's own lstd. We could somehow tell it to share the state but that gets messy.
// Instead we let the exe manage the main window, the dll can create and do it's own stuff with windows, etc.
struct main_window_api {
	int2(*GetSize)();
	int2(*GetPos)();
	int2(*GetCursorPos)();

    bool (*IsVisible)();

    bool (*IsVsync)();
    void (*SetVsync)(bool enabled);

	u32 (*GetFlags)();

    void (*GrabMouse)();
    void (*UngrabMouse)();
};

// The permanent state of the game.
// This does not get affected on reload.
struct memory {
    // Gets set to true when the game code has been reloaded during the frame
    // (automatically set to false the next frame).
    // Gets set the first time the game loads as well!
    bool ReloadedThisFrame = false;

    main_window_api MainWindow;

    // This gets set by the DLL.
    // Tells the exe to reload.
    bool RequestReloadNextFrame = false;

    // Our target FPS by default is 60. If the PC we are running on doesn't manage to hit that, we need to reduce
    // it. Then the frame delta must change. So we shouldn't hardcode 1/60 spf (e.g. for physics calculations)
    // and instead use this variable managed by the exe.
    f32 FrameDelta;

    // The ImGui context must be shared
    void *ImGuiContext;

    // ... and the allocators
    ImGuiMemAllocFunc ImGuiMemAlloc;
    ImGuiMemFreeFunc ImGuiMemFree;

    // The exe provides us with these allocators.
    allocator PersistentAlloc;
    allocator TemporaryAlloc;

    // Keeps track of allocated pointers with a string identifier as a key. This is ok because we use this
    // table only when we reload the dll. This maps the global pointers in the dll to the ones stored in
    // this table. If they don't exist (running for the first time) we allocate a new one and put it in.
    hash_table<string, void *> States;

    // We need states to be allocated in the .exe, since the .exe is gonna free them in the end.
    // @Cleanup Why does this function need to be in the exe?
    void *(*GetStateImpl)(string name, s64 size, bool *created);

    // Call this function to manage a global pointer using our table of states above.
    // If the thing with type T doesn't exist in the table, we allocate it and insert it
    // otherwise we return whatever is stored with that name.
    template <typename T>
    T *get_state(string name) {
        bool created = false;
        auto *result = (T *) GetStateImpl(name, sizeof(T), &created);
        if (created) {
            *result = T{};
        }
        return result;
    }
};

// These two hold global state and can be accessed from anywhere.
// Should only get modified by 1 thread at any time.
inline memory *Memory     = null;
inline graphics *Graphics = null;

// Used to make Memory->get_state less error-prone and less verbose.
// The name used is just a string version of the variable name.
#define MANAGE_GLOBAL_VARIABLE(name) name = Memory->get_state<remove_pointer_t<decltype(name)>>(#name)

// This is the API with which the exe and the dll interface.
// UPDATE_AND_RENDER is the main one which runs at 60 fps or so (we VSYNC otherwise calculations e.g. physics will be wrong).
// MAIN_WINDOW_EVENT is there to listen for window events without having to connect/disconnect
//                   event callbacks from the dll which is annoying and bug-prone.

#define UPDATE_AND_RENDER(name, ...) void name(memory *m, graphics *g)
typedef UPDATE_AND_RENDER(update_and_render_func);

#define MAIN_WINDOW_EVENT(name, ...) bool name(event e)
typedef MAIN_WINDOW_EVENT(main_window_event_func);
