#pragma once

#include <driver.h>

#include "ast.h"

struct camera {
    //
    // Note: We used to use these properties to build a view matrix which transforms all geometry in the viewport
    // but now instead we build the geometry using these properties - this has many benefits -> lines don't get blurry
    // at close zoom levels, line thickness doesn't change at different zoom levels, etc. Everything is much cleaner.
    // We can do this because we know we are drawing a graph and not arbitrary geometry, which means we don't need all
    // the flexibility that a camera-view-matrix system provides us with.
    //
    float2 Position;
    // f32 Roll; Should we even support this?
    float2 Scale;

    f32 PanSpeed;
    f32 ZoomSpeed;

    f32 ScaleMin, ScaleMax;

    bool Panning;

    s32 ScrollY;  // This gets updated if the mouse is in the viewport, then we zoom the camera
};

void camera_reinit(camera *cam);
void camera_reset_constants(camera *cam);
void camera_update(camera *cam);

//
// We support graphing multiple functions at once.
//
struct function_entry {
    static constexpr s64 FORMULA_INPUT_BUFFER_SIZE = 16_KiB;
    char Formula[FORMULA_INPUT_BUFFER_SIZE]{};

    string FormulaMessage;
    ast *FormulaRoot = null;

    hash_table<char32_t, f64> Parameters;

    float4 Color = {1.0, 0.2f, 0.3f, 0.8f};

    // @TODO:
    // - Domain
    // - Parse y= or x=

    // See note in graph_state.
    u32 ImGuiID = 0;

    function_entry();
};

inline void free(function_entry &entry) {
    if (entry.FormulaRoot) free_ast(entry.FormulaRoot);
    free(entry.FormulaMessage);

    free(entry.Parameters);
}

struct graph_state {
    float4 ClearColor = {0.92f, 0.92f, 0.92f, 1.0f};
    camera Camera;

    float2 ViewportPos, ViewportSize;  // Set in viewport.cpp, needed to determine if mouse is in the viewport
    float2 LastMouse;                  // Gets calculated in camera.cpp @Cleanup Use events?

    bool DisplayAST = false;

    array<function_entry> Functions;

    // Used by stuff that gets drawn in ImGui.
    // Each element must have a unique ID - usually that gets determined by the display string.
    // However when we have elements with the same dispay string, state gets shared between the two.
    // That's why the abstract syntax tree nodes and function entries get a unique id that is used when drawing.
    //
    // This is not thread-safe since for now we are running in a single thread. Otherwise use atomic_increment.
    u32 NextImGuiID = 0;
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

inline graph_state *GraphState = null;

inline bool point_in_rect(float2 p, float2 min, float2 max) { return p.x > min.x && p.y > min.y && p.x < max.x && p.y < max.y; }

inline bool mouse_in_viewport() {
    float2 vpPos = GraphState->ViewportPos - (float2) Memory->MainWindow->get_pos();
    float2 vpSize = GraphState->ViewportSize;
    return point_in_rect(GraphState->LastMouse, vpPos, vpPos + vpSize);
}