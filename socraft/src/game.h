#pragma once

#include <driver.h>

struct camera;
void camera_reinit(camera *cam);

struct camera {
    camera() { camera_reinit(this); }  // Do this once when running for the first time
};

void camera_reset_constants(camera *cam);
void camera_update(camera *cam);

struct socraft_state {
    float4 ClearColor = {0.92f, 0.92f, 0.92f, 1.0f};
    camera Camera;
};

void reload_global_state();

void ui_main();
void ui_scene_properties();

inline void render_ui() {
    ui_main();
    ui_scene_properties();
}

void render_viewport();

inline socraft_state *Game = null;

