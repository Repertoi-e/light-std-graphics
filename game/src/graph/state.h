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

    f32 ScaleMin, ScaleMax;

    bool Panning;

    s32 ScrollY;  // This gets updated if the mouse is in the viewport, then we zoom the camera
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

    v2 ViewportPos, ViewportSize;  // Set in viewport.cpp, needed to determine if mouse is in the viewport
    v2 LastMouse;                  // Gets calculated in camera.cpp @Cleanup Use events?

    bool DisplayAST = false;

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

inline bool point_in_rect(v2 p, v2 min, v2 max) { return p.x > min.x && p.y > min.y && p.x < max.x && p.y < max.y; }

inline bool mouse_in_viewport() {
    v2 vpPos = GameState->ViewportPos - (v2) GameMemory->MainWindow->get_pos();
    v2 vpSize = GameState->ViewportSize;
    return point_in_rect(GameState->LastMouse, vpPos, vpPos + vpSize);
}