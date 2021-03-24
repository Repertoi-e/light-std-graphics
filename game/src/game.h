#pragma once

#include <lstd/io.h>
#include <lstd/memory/array.h>
#include <lstd/memory/free_list_allocator.h>
#include <lstd/memory/hash_table.h>
#include <lstd/os.h>
#include <lstd_graphics/graphics.h>
#include <lstd_graphics/video.h>

using namespace lstd;  // The library is in it's own namespace for the sake of not being too invasive. I don't really want a namespace though!

// LE_GAME_API is used to export functions from the game dll
#if OS == WINDOWS
#if defined LE_BUILDING_GAME
#define LE_GAME_API extern "C" __declspec(dllexport)
#else
#define LE_GAME_API __declspec(dllimport)
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
struct game_memory {
    // Gets set to true when the game code has been reloaded during the frame
    // (automatically set to false the next frame).
    // Gets triggered the first time the game loads as well!
    bool ReloadedThisFrame = false;

    // This gets set by the dll, tells the exe to reload.
    bool RequestReloadNextFrame = false;

    window *MainWindow = null;

    // The ImGui context must be shared, because we submit the geometry to the GPU in the exe
    void *ImGuiContext = null;

    // This also needs to be shared... @TODO: Comment on sharing the memory between the two modules
#if defined DEBUG_MEMORY
    debug_memory *DEBUG_memory = null;
#endif

    // The exe provides us with a free list allocator that is faster than malloc and is suited for general purpose
    // allocations. Historically the game initialized the allocator but it's better if we allow the exe to use it for
    // imgui and thus we don't use malloc anywhere now. We also support setting the allocated block size with a command
    // line argument (before you had to edit the game code and reload the entire game anyways).
    allocator Alloc;

    // You can edit the placement policy for the allocator directly in here - find first or find_best. The first one
    // finds the first free memory block but might cause fragmentation and waste space, while the second one is slower
    // and finds a block which best suits the request.
    free_list_allocator_data *AllocData;

    // Keeps track of allocated pointers with an identifier as a key. This is not slow because we use this
    // table only when we reload and we need to map the global pointers in the dll to these (if they exists
    // at all, otherwise we allocate a new one and put it in this table).
    hash_table<string, void *> States;

    // We need states to be allocated in the .exe, since the .exe is gonna free them in the end.
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

    // Our target FPS by default is 60. If the PC we are running on doesn't manage to hit that, we need to reduce
    // it. Then the frame delta must change. So we shouldn't hardcode 1/60 spf (e.g. for physics calculations) everywhere
    // and instead use this variable managed by the exe.
    f32 FrameDelta;
};

// Used to make GameMemory->get_state less error-prone and less verbose. The name used is just a string version of the variable name.
#define MANAGE_GLOBAL_VARIABLE(variable) variable = GameMemory->get_state<types::remove_pointer_t<decltype(variable)>>(#variable)

// This is the API with which the exe and the dll interface
#define GAME_UPDATE_AND_RENDER(name, ...) void name(game_memory *memory, graphics *g)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render_func);

#define GAME_MAIN_WINDOW_EVENT(name, ...) bool name(const event &e)
typedef GAME_MAIN_WINDOW_EVENT(game_main_window_event_func);

inline game_memory *GameMemory = null;
inline graphics *Graphics = null;
