#include "game.h"

void reload_global_state() {
    // Copy state from exe
    assert(Memory->ImGuiContext);
    ImGui::SetCurrentContext((ImGuiContext *) Memory->ImGuiContext);
    ImGui::SetAllocatorFunctions(Memory->ImGuiMemAlloc, Memory->ImGuiMemFree);

    auto newContext  = Context;
    newContext.Alloc = Memory->PersistentAlloc;
    // newContext.AllocAlignment = 16;  // For SIMD
    newContext.Log = &cout;

    *const_cast<allocator *>(&TemporaryAllocator) = Memory->TemporaryAlloc;

    OVERRIDE_CONTEXT(newContext);

    // Global variables that need to persist DLL reloads
    MANAGE_GLOBAL_VARIABLE(Game);
}

DRIVER_API UPDATE_AND_RENDER(update_and_render, memory *m, graphics *g) {
    if (m->ReloadedThisFrame) {
        Memory   = m;
        Graphics = g;

        reload_global_state();
    }

    auto *cam = &Game->Camera;
    camera_update(cam);

    if (Memory->MainWindow.is_visible()) {
        render_ui();
        render_viewport();
    }

    free_all(TemporaryAllocator);
}

DRIVER_API MAIN_WINDOW_EVENT(main_window_event, const event &e) {
    if (!Game) return false;
    assert(e.Window == Memory->MainWindow);

    return false;
}
