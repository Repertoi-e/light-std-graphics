#pragma once

#include <game.h>

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

    camera Camera;

    game_memory *Memory;
};

void reload_global_state();

void editor_main();
void editor_scene_properties();

void viewport_render();

inline game_state *GameState = null;
