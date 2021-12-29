#pragma once

#include <driver.h>

struct camera {
    float3 Position = {};  // x, y, z
    float3 Rotation = {};  // pitch, roll, yaw

    float4x4 PerspectiveMatrix;
    float4x4 ViewMatrix;
};

void camera_update();
void camera_init_perspective_matrix(f32 aspect);

constexpr s64 CHUNK_SIZE   = 16;
constexpr s64 CHUNK_HEIGHT = 256;

struct chunk_vertex {
    float3 Position;
    float4 Color;
};

struct chunk {
    s16 X, Y;

    chunk *N = null;
    chunk *E = null;
    chunk *S = null;
    chunk *W = null;

    bool Dirty;

    byte Blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_HEIGHT];

    gbuffer VB, IB;
    u32 Indices = 0;
};

inline chunk *get_chunk(s16 x, s16 y) {
    auto *c = malloc<chunk>();

    c->X     = x;
    c->Y     = y;
    c->Dirty = true;

    fill_memory(c->Blocks, 0, sizeof(c->Blocks));

    return c;
}

struct socraft_state {
    float4 ClearColor = {0.65f, 0.87f, 0.95f, 1.0f};

    bool UI = true;

    bool ViewportDirty   = false;  // When the UI window holding the texture has been resized we need to reinit the texture
    texture_2D *Viewport = null;   // Render target
    int2 ViewportSize    = {};     // Size of the texture in pixels

    camera Camera;

    chunk *PlayerChunk = null;

    gbuffer ChunkUB;  // The MVP matrix that gets uploaded to the GPU for each chunk drawn
    shader ChunkShader;
};

void reload_global_state();

void ui_main();
void ui_scene_properties();
void ui_viewport();

inline void render_ui() {
    ui_main();
    ui_scene_properties();
    ui_viewport();
}

void render_world();

inline socraft_state *Game = null;
