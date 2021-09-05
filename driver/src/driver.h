#pragma once

#include <lstd/io.h>
#include <lstd/memory/array.h>
#include <lstd/memory/hash_table.h>
#include <lstd_graphics/graphics.h>
#include <lstd_graphics/video.h>

using namespace lstd;  // The library is in it's own namespace for the sake of not being too invasive. I don't really want a namespace though!

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

// The permanent state of the game.
// This does not get affected on reload.
struct memory {
    // Gets set to true when the game code has been reloaded during the frame
    // (automatically set to false the next frame).
    // Gets triggered the first time the game loads as well!
    bool ReloadedThisFrame = false;

    // This gets set by the dll, tells the exe to reload.
    bool RequestReloadNextFrame = false;

    window *MainWindow = null;

    // Our target FPS by default is 60. If the PC we are running on doesn't manage to hit that, we need to reduce
    // it. Then the frame delta must change. So we shouldn't hardcode 1/60 spf (e.g. for physics calculations) everywhere
    // and instead use this variable managed by the exe.
    f32 FrameDelta;

    // The ImGui context must be shared, because we submit the geometry to the GPU in the exe
    void *ImGuiContext = null;

    // This also needs to be shared... @TODO: Comment on sharing the memory between the two modules
#if defined DEBUG_MEMORY
    debug_memory *DEBUG_memory = null;
#endif

    // The exe provides us with these allocators.
    allocator Alloc;
    allocator TempAlloc;

    // Keeps track of allocated pointers with an identifier as a key. This is not slow because we use this
    // table only when we reload and we need to map the global pointers in the dll to these (if they exist
    // at all, otherwise we allocate a new one and put it in this table).
    hash_table<string, void *> States;

    // We need states to be allocated in the .exe, since the .exe is gonna free them in the end.
    // @Cleanup Why does this function need to be in the exe?
    void *(*GetStateImpl)(const string &name, s64 size, bool *created);

    // Call this function to manage a global pointer using our table of states above.
    // If the thing with type T doesn't exist in the table, we allocate it and insert it
    // otherwise we return whatever is stored with that name.
    template <typename T>
    T *get_state(const string &name) {
        bool created = false;
        auto *result = (T *) GetStateImpl(name, sizeof(T), &created);
        if (created) new (result) T;  // initialize
        return result;
    }
};

// Used to make Memory->get_state less error-prone and less verbose. The name used is just a string version of the variable name.
#define MANAGE_GLOBAL_VARIABLE(name) name = Memory->get_state<types::remove_pointer_t<decltype(name)>>(#name)

// This is the API with which the exe and the dll interface
#define UPDATE_AND_RENDER(name, ...) void name(memory *m, graphics *g)
typedef UPDATE_AND_RENDER(update_and_render_func);

#define MAIN_WINDOW_EVENT(name, ...) bool name(const event &e)
typedef MAIN_WINDOW_EVENT(main_window_event_func);

inline memory *Memory = null;
inline graphics *Graphics = null;
