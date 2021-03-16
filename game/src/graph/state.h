#pragma once

#include <game.h>

struct camera {
    v2 Position;
    v2 Scale;
    f32 Roll;

    f32 PanSpeed;
    f32 RotationSpeed;
    f32 ZoomSpeed;

    f32 ZoomMin, ZoomMax;
};

void camera_reinit(camera *cam);
void camera_reset_constants(camera *cam);
void camera_update(camera *cam);

struct game_state {
    v4 ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};

    camera Camera;

    m33 ViewMatrix;
    m33 InverseViewMatrix;

    ImDrawList *ViewportDrawlist;
    v2 ViewportPos;
    v2 ViewportSize;

    bool EditorShowShapeType = false;
    bool EditorShowDrawAABB = false;
    bool EditorShowPositionalCorrection = false;
    bool EditorShowDebugIntersections = false;
    bool EditorShowIterations = false;

    s32 EditorShapeType = false;
    bool EditorDrawAABB = false;
    bool EditorPositionalCorrection = false;
    bool EditorDebugIntersections = false;
    s32 EditorIterations = 5;

    // We scale coordinates by this amount to appear better on the screen
    f32 PixelsPerMeter = 50;

    game_memory *Memory;
};

void reload_global_state();

void editor_main();
void editor_scene_properties();

void viewport_render();

inline game_state *GameState = null;
