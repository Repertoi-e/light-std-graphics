#pragma once

#include <driver.h>

struct camera {
    float3 Position = {0, 0, -4};  // x, y, z
    float3 Rotation = {};  // pitch, roll, yaw

    float4x4 PerspectiveMatrix;
    float4x4 ViewMatrix;

    f32 Sensitivity = 0.003f;
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
    // byte Blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    
    bool Dirty;
    gbuffer VB, IB;
    u32 Indices = 0;
};

inline chunk *get_chunk() {
    // @TODO: Chunk pool
    auto *c = malloc<chunk>();
    c->Dirty = true;
    // fill_memory(c->Blocks, 0, sizeof(c->Blocks));
    return c;
}

struct player {
   //  float3 Position;
    int3 Chunk;
};

struct socraft_state {
    float4 ClearColor = {0.65f, 0.87f, 0.95f, 1.0f};

    bool UI = false;
    bool MouseGrabbed = false;

    int2 ViewportSize    = {};     // Size of the texture in pixels
    texture_2D *Viewport = null;   // Render target
    bool ViewportDirty   = false;  // When the UI window holding the texture has been resized we need to reinit the texture

    camera Camera;
    player Player;
    
    hash_table<int3, chunk *> Chunks;
    
    bool RenderInitted = false;
    shader ChunkShader;
    gbuffer ChunkShaderUB;  // The MVP matrix that gets uploaded to the GPU for each chunk mesh
};

inline socraft_state *Game = null;

void reload_global_state();

bool camera_event(event e);

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

