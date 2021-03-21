#pragma once

#include <game.h>

#include "ast.h"

struct camera {
    //
    // Note: We used to use these properties to build a view matrix which transforms all geometry in the viewport
    // but now instead we build the geometry using these properties - this has many benefits -> lines don't get blurry
    // at close zoom levels, line thickness doesn't change at different zoom levels, etc. Everything is much cleaner.
    // We can do this because we know we are drawing a graph and not arbitrary geometry, which means we don't need all
    // the flexibility that a camera-view-matrix system provides us with.
    //
    v2 Position;
    // f32 Roll; Should we even support this?
    v2 Scale;

    f32 PanSpeed;
    f32 ZoomSpeed;

    f32 ZoomMin, ZoomMax;
};

void camera_reinit(camera *cam);
void camera_reset_constants(camera *cam);
void camera_update(camera *cam);

struct game_state {
    v4 ClearColor = {0.92f, 0.92f, 0.92f, 1.0f};

    static constexpr s64 FORMULA_INPUT_BUFFER_SIZE = 100000;
    char Formula[FORMULA_INPUT_BUFFER_SIZE]{};

    string FormulaMessage;
    ast *FormulaRoot = null;

    camera Camera;
};

void reload_global_state();

void ui_main();
void ui_scene_properties();
void ui_functions();

inline void render_ui() {
    ui_main();
    ui_scene_properties();
    ui_functions();
}

void render_viewport();

inline game_state *GameState = null;
