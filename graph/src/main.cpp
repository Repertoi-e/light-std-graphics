#include <driver.h>
#include "state.h"

DRIVER_API UPDATE_AND_RENDER(update_and_render, memory *m, graphics *g) {
    if (m->ReloadedThisFrame) {
        Memory = m;
        Graphics = g;

        reload_global_state();

        // Ensure at least one function entry exists
        make_dynamic(&GraphState->Functions, 10);
        if (!GraphState->Functions) add(&GraphState->Functions, function_entry{});
    }

    auto *cam = &GraphState->Camera;
    camera_update(cam);

    if (Memory->MainWindow.IsVisible()) {
        render_ui();
        render_viewport();
    }

    free_all(TemporaryAllocator);
}

DRIVER_API MAIN_WINDOW_EVENT(main_window_event, const event &e) {
    if (!GraphState) return false;

    if (e.Type == event::Mouse_Wheel_Scrolled) {
        if (mouse_in_viewport()) {
            GraphState->Camera.ScrollY += e.ScrollY;
            return true;  // We handled the event, don't propagate to other listeners
        }
    }

    return false;
}
