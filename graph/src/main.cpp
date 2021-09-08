#include "pch.h"
#include "state.h"

DRIVER_API UPDATE_AND_RENDER(update_and_render, memory *m, graphics *g) {
    if (m->ReloadedThisFrame) {
        Memory = m;
        Graphics = g;

        reload_global_state();

        // Ensure at least one function entry exists
        if (!GraphState->Functions) array_append(GraphState->Functions);
    }

    auto *cam = &GraphState->Camera;
    camera_update(cam);

    if (Memory->MainWindow.is_visible()) {
        render_ui();
        render_viewport();
    }

    free_all(Context.TempAlloc);
}

DRIVER_API MAIN_WINDOW_EVENT(main_window_event, const event &e) {
    if (!GraphState) return false;

    assert(e.Window == Memory->MainWindow);

    if (e.Type == event::Mouse_Wheel_Scrolled) {
        if (mouse_in_viewport()) {
            GraphState->Camera.ScrollY += e.ScrollY;
            return true;  // We handled the event, don't propagate to other listeners
        }
    }

    return false;
}
