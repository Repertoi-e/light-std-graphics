#include "state.h"

#include <driver.h>

import lstd.os;

extern "C" bool lstd_init_global() { return false; }

void copy_state_from_exe() {
    // @Cleanup
#if defined DEBUG_MEMORY
    DEBUG_memory = Memory->DEBUG_memory;
#endif

    assert(Memory->ImGuiContext);
    ImGui::SetCurrentContext((ImGuiContext *) Memory->ImGuiContext);

    // We also tell imgui to use our allocator (we tell imgui to not implement default ones)
    // We mark allocations as LEAK because any leftover are handled by the exe and we don't to report them when this .dll terminates.
    ImGui::SetAllocatorFunctions([](size_t size, void *) { return (void *) malloc<char>({.Count = (s64) size, .Alloc = Memory->PersistentAlloc, .Options = LEAK}); },
                                 [](void *ptr, void *) { free(ptr); });

    auto newContext           = Context;
    newContext.Alloc          = Memory->PersistentAlloc;
    newContext.AllocAlignment = 16;  // For SIMD
    newContext.Log            = &cout;

    *const_cast<allocator *>(&TemporaryAllocator) = Memory->TemporaryAlloc;

    OVERRIDE_CONTEXT(newContext);
}

void reload_global_state() {
    copy_state_from_exe();

    MANAGE_GLOBAL_VARIABLE(GraphState);
}

// Defined here since they need to access GraphState
function_entry::function_entry() : ImGuiID(++GraphState->NextImGuiID) {}
ast::ast(type t, ast *left, ast *right) : Type(t), Left(left), Right(right), ImGuiID(++GraphState->NextImGuiID) {}
