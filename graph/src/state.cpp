#include "state.h"

#include <driver.h>

import lstd.os;

extern "C" bool lstd_init_global() { return false; }

void copy_state_from_exe() {
    assert(Memory->ImGuiContext);
    ImGui::SetCurrentContext((ImGuiContext *) Memory->ImGuiContext);
    ImGui::SetAllocatorFunctions(Memory->ImGuiMemAlloc, Memory->ImGuiMemFree);

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
