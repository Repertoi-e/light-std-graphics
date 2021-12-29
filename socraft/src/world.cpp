#include "game.h"

void rebuild_chunk_geometry(chunk *c) {
    free_buffer(&c->VB);
    free_buffer(&c->IB);

    chunk_vertex vertices[] =
        {
            {float3(-1.0f, 1.0f, -1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)},
            {float3(1.0f, 1.0f, -1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)},
            {float3(1.0f, 1.0f, 1.0f), float4(0.0f, 1.0f, 1.0f, 1.0f)},
            {float3(-1.0f, 1.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)},
            {float3(-1.0f, -1.0f, -1.0f), float4(1.0f, 0.0f, 1.0f, 1.0f)},
            {float3(1.0f, -1.0f, -1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f)},
            {float3(1.0f, -1.0f, 1.0f), float4(1.0f, 1.0f, 1.0f, 1.0f)},
            {float3(-1.0f, -1.0f, 1.0f), float4(0.0f, 0.0f, 0.0f, 1.0f)},
        };

    u32 indices[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6
    };

    auto vbSize = sizeof(vertices);
    graphics_init_buffer(&c->VB, Graphics, gbuffer_type::Vertex_Buffer, gbuffer_usage::Dynamic, vbSize);

    gbuffer_layout layout;
    make_dynamic(&layout, 2);
    add(&layout, layout_element("POSITION", gtype::F32_3));
    add(&layout, layout_element("COLOR", gtype::F32_4));
    set_layout(&c->VB, layout);

    free(layout.Data);

    auto ibSize = sizeof(indices);
    graphics_init_buffer(&c->IB, Graphics, gbuffer_type::Index_Buffer, gbuffer_usage::Dynamic, ibSize);

    auto *vb = (ImDrawVert *) map(&c->VB, gbuffer_map_access::Write_Discard_Previous);
    copy_memory_fast(vb, vertices, sizeof(vertices));
    unmap(&c->VB);

    auto *ib = (u32 *) map(&c->IB, gbuffer_map_access::Write_Discard_Previous);
    copy_memory_fast(ib, indices, sizeof(indices));
    unmap(&c->IB);
}

void render_world() {
    Graphics->clear_color(Game->ClearColor);

    Game->ChunkShader.bind();

    auto *c = Game->PlayerChunk;
    if (c->Dirty) {
        rebuild_chunk_geometry(c);
        c->Dirty = false;
    }

    if (c->Indices) {
        bind_vb(&c->VB, primitive_topology::TriangleList);
        bind_ib(&c->IB);

        auto *ub = map(&Game->ChunkUB, gbuffer_map_access::Write_Discard_Previous);
        auto mvp = math::mul(Game->Camera.ViewMatrix, Game->Camera.PerspectiveMatrix);
        copy_memory_fast(ub, &mvp, sizeof(float4x4));
        unmap(&Game->ChunkUB);

        bind_ub(&Game->ChunkUB, shader_type::Vertex_Shader, 0);

        Graphics->draw_indexed(c->Indices, 0, 0);

        unbind(&Game->ChunkUB);
    }
}
