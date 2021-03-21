#if !defined LE_BUILDING_GAME
#error Error
#endif

#include "state.h"

LE_GAME_API GAME_UPDATE_AND_RENDER(game_update_and_render, game_memory *memory, graphics *g) {
    if (memory->ReloadedThisFrame) {
        GameMemory = memory;
        Graphics = g;

        reload_global_state();

        auto *cam = &GameState->Camera;
        camera_reinit(cam);
    }

    auto *cam = &GameState->Camera;
    camera_update(cam);

    if (GameMemory->MainWindow->is_visible()) {
        render_ui();
        render_viewport();
    }

    free_all(Context.Temp);
}

LE_GAME_API GAME_MAIN_WINDOW_EVENT(game_main_window_event, const event &e) {
    if (!GameState) return false;
    return false;
}
