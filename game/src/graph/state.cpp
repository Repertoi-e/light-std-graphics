#include "state.h"

extern "C" bool lstd_init_global() { return false; }

void copy_state_from_exe() {
    auto *c = (context *) &Context;    // @Constcast
    DefaultAlloc = GameMemory->Alloc;  // Switch our default allocator from malloc to the one the exe provides us with
    c->Alloc = GameMemory->Alloc;
    c->AllocAlignment = 16;  // For SIMD

#if defined DEBUG_MEMORY
    DEBUG_memory = GameMemory->DEBUG_memory;
#endif

    assert(GameMemory->ImGuiContext);
    ImGui::SetCurrentContext((ImGuiContext *) GameMemory->ImGuiContext);

    // We also tell imgui to use our allocator (we tell imgui to not implement default ones)
    // We mark allocations as LEAK because any leftover are handled by the exe and we don't to report them when this .dll terminates.
    ImGui::SetAllocatorFunctions([](size_t size, void *) { return (void *) allocate_array<char>(size, {.Alloc = GameMemory->Alloc, .Options = LEAK}); },
                                 [](void *ptr, void *) { lstd::free(ptr); });
}

void reload_global_state() {
    copy_state_from_exe();

    MANAGE_STATE(GameState);
}
