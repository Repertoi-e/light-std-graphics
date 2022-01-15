#include "game.h"

void rebuild_chunk_geometry(chunk *c) {
    free_buffer(&c->VB);
    free_buffer(&c->IB);

    chunk_vertex vertices[] =
        {
            {float3(-1.0f, -1.0f, -1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)},
            {float3(1.0f, -1.0f, -1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)},
            {float3(1.0f, 1.0f, -1.0f), float4(0.0f, 1.0f, 1.0f, 1.0f)},
            {float3(-1.0f, 1.0f, -1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)},
            {float3(-1.0f, -1.0f, 1.0f), float4(1.0f, 0.0f, 1.0f, 1.0f)},
            {float3(1.0f, -1.0f, 1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f)},
            {float3(1.0f, 1.0f, 1.0f), float4(1.0f, 1.0f, 1.0f, 1.0f)},
            {float3(-1.0f, 1.0f, 1.0f), float4(0.0f, 0.0f, 0.0f, 1.0f)},
        };

    u32 indices[] =
    {
        // front
        2, 1, 0,
		0, 3, 2,
		// right
		6, 5, 1,
		1, 2, 6,
		// back
		5, 6, 7,
		7, 4, 5,
		// left
		3, 0, 4,
		4, 7, 3,
		// bottom
		1, 5, 4,
		4, 0, 1,
		// top
		6, 2, 3,
		3, 7, 6
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

    auto *vb = map(&c->VB, gbuffer_map_access::Write_Discard_Previous);
    copy_memory_fast(vb, vertices, sizeof(vertices));
    unmap(&c->VB);

    auto *ib = map(&c->IB, gbuffer_map_access::Write_Discard_Previous);
    copy_memory_fast(ib, indices, sizeof(indices));
    unmap(&c->IB);

    c->Indices = sizeof(indices) / sizeof(u32);
}

void ensure_render_initted() {
    if (Game->RenderInitted) return;

    // @TODO: Asset system + reloading
    Game->ChunkShader.Name = "Chunk Shader";
    Game->ChunkShader.init_from_file(Graphics, "data/Chunk.hlsl");

    graphics_init_buffer(&Game->ChunkShaderUB, Graphics, gbuffer_type::Shader_Uniform_Buffer, gbuffer_usage::Dynamic, sizeof(float4x4));

    Game->RenderInitted = true;
}

void render_world() {
    Graphics->clear_color(Game->ClearColor);

    /*
    if (Game->VisibleChunks) {
        Game->ChunkShader.bind();

        auto *c = Game->PlayerChunk;
        if (c->Dirty) {
            rebuild_chunk_geometry(c);
            c->Dirty = false;
        }

        if (c->Indices) {
            bind_vb(&c->VB, primitive_topology::TriangleList);
            bind_ib(&c->IB);

            auto *ub = map(&Game->ChunkShaderUB, gbuffer_map_access::Write_Discard_Previous);
            auto mvp = math::mul(Game->Camera.PerspectiveMatrix, Game->Camera.ViewMatrix);
            copy_memory_fast(ub, &mvp, sizeof(float4x4));
            unmap(&Game->ChunkShaderUB);

            bind_ub(&Game->ChunkShaderUB, shader_type::Vertex_Shader, 0);

            Graphics->draw_indexed(c->Indices, 0, 0);

            unbind(&Game->ChunkShaderUB);
        }
    }*/
}
