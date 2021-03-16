#include "state.h"

void reload_global_state() {
    DEBUG_memory_info::CheckForLeaksAtTermination = true;

    auto *c = (context *) &Context;  // @Constcast
    c->Alloc = GameMemory->Alloc;    // Switch our default allocator from malloc to the one the exe provides us with
    c->AllocAlignment = 16;          // For SIMD

    // We need to use the exe's imgui context, because we submit the geometry to the GPU there
    assert(GameMemory->ImGuiContext);
    ImGui::SetCurrentContext((ImGuiContext *) GameMemory->ImGuiContext);

    // We also tell imgui to use our allocator (we tell imgui to not implement default ones)
    // We mark allocations as LEAK because any leftover are handled by the exe and we don't to report them when this .dll terminates.
    ImGui::SetAllocatorFunctions([](size_t size, void *) { return (void *) allocate_array<char>(size, {.Alloc = GameMemory->Alloc, .Options = LEAK}); },
                                 [](void *ptr, void *) { lstd::free(ptr); });

    MANAGE_STATE(GameState);
    MANAGE_STATE(AssetCatalog);

    // We need these in python.pyb
    GameState->Memory = GameMemory;

    AssetCatalog->ensure_initted("data/");
}
