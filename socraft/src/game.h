#pragma once

#include <driver.h>

struct camera {
    float3 Position = {0, 0, -4};  // x, y, z
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
    byte Blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_HEIGHT];

    // Mesh:
    bool Dirty;
    gbuffer VB, IB;
    u32 Indices = 0;
};

inline chunk *get_chunk() {
    auto *c = malloc<chunk>();
    c->Dirty = true;
    fill_memory(c->Blocks, 0, sizeof(c->Blocks));
    return c;
}

struct player {
    float3 Position;
};

#define CHUNK_RANGE 3

struct socraft_state {
    float4 ClearColor = {0.65f, 0.87f, 0.95f, 1.0f};

    bool UI = false;
    bool MouseGrabbed = false;

    int2 ViewportSize    = {};     // Size of the texture in pixels
    texture_2D *Viewport = null;   // Render target
    bool ViewportDirty   = false;  // When the UI window holding the texture has been resized we need to reinit the texture

    camera Camera;
    player Player;
    
    hash_table<int2, chunk *> Chunks;
    
    bool RenderInitted = false;
    shader ChunkShader;
    gbuffer ChunkShaderUB;  // The MVP matrix that gets uploaded to the GPU for each chunk mesh
};

void grab_mouse();
void ungrab_mouse();

void reload_global_state();

void ui_main();
void ui_scene_properties();
void ui_viewport();

inline void render_ui() {
    ui_main();
    ui_scene_properties();
    ui_viewport();
}

void ensure_render_initted();
void render_world();

inline socraft_state *Game = null;
