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

    // @Cleanup
    *const_cast<allocator *>(&TemporaryAllocator) = Memory->TemporaryAlloc;

    OVERRIDE_CONTEXT(newContext);

    // Global variables that need to persist DLL reloads,
    // if any of these data structs change the engine needs to be restarted.
    MANAGE_GLOBAL_VARIABLE(Game);
}

void reinit_render_target(s32 width, s32 height) {
    if (!Game->Viewport) Game->Viewport = malloc<texture_2D>();

    Game->Viewport->release();
    Game->Viewport->init_as_render_target(Graphics, width, height);
}

DRIVER_API UPDATE_AND_RENDER(update_and_render, memory *m, graphics *g) {
    if (m->ReloadedThisFrame) {
        Memory   = m;
        Graphics = g;

        reload_global_state();

        // When we don't draw the editor UI and we need to generate the perspective matrix the first time 
        if (!Game->UI) {
            int2 size = Memory->MainWindow.GetSize();
            camera_init_perspective_matrix((f32) size.x / size.y);
        }

        ensure_render_initted();
    }

    camera_update();

    if (Memory->MainWindow.IsVisible()) {
        if (Game->ViewportDirty) {
            reinit_render_target(Game->ViewportSize.x, Game->ViewportSize.y);
            camera_init_perspective_matrix((f32) Game->ViewportSize.x / Game->ViewportSize.y);
            Game->ViewportDirty = false;
        }

        if (Game->UI) {
            if (Game->Viewport) {
                Graphics->set_custom_render_target(Game->Viewport);
                
                g->set_cull_mode(cull::Back);
                render_world();

                Graphics->set_custom_render_target(null);
            }
            render_ui();
        } else {
            // No UI, just render to the screen
            g->set_cull_mode(cull::Back);
            render_world();
        }
    }

    free_all(TemporaryAllocator);
}

DRIVER_API MAIN_WINDOW_EVENT(main_window_event, event e) {
    if (!Game) return false;
    
    if (e.Type == event::Key_Pressed) {
        if (e.KeyCode == Key_F1) {
            Game->UI = !Game->UI;
        } else if (e.KeyCode == Key_Escape) {
            if (Game->MouseGrabbed) {
                Memory->MainWindow.UngrabMouse();
                Game->MouseGrabbed = false;
            }
        }
    }

    if (!Game->UI && e.Type == event::Window_Framebuffer_Resized) {
        camera_init_perspective_matrix((f32) e.Width / e.Height);
    }

    if (!Game->UI && !Game->MouseGrabbed && e.Type == event::Mouse_Button_Pressed) {
        Memory->MainWindow.GrabMouse();
        Game->MouseGrabbed = true;
    }

    if (Game->MouseGrabbed && camera_event(e)) {
        return true;
    }

    return false;
}
