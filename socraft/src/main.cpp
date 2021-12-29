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

        // temp, put this initialization somewhere better?
        if (!Game->PlayerChunk) {
            Game->PlayerChunk = get_chunk(0, 0);

            Game->ChunkShader.Name = "Chunk Shader";
            Game->ChunkShader.init_from_file(g, "data/Chunk.hlsl");

            graphics_init_buffer(&Game->ChunkUB, g, gbuffer_type::Shader_Uniform_Buffer, gbuffer_usage::Dynamic, sizeof(float4x4));
        }
    }

    camera_update();

    if (Memory->MainWindow.is_visible()) {
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
            render_world();
        }
    }

    free_all(TemporaryAllocator);
}

DRIVER_API MAIN_WINDOW_EVENT(main_window_event, event e) {
    if (!Game) return false;
    assert(e.Window == Memory->MainWindow);

    if (e.Type == event::Key_Pressed && e.KeyCode == Key_F1) {
        Game->UI = !Game->UI;
    }

    return false;
}
