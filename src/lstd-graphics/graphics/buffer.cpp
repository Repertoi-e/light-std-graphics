#include "buffer.h"

#include "api.h"

LSTD_BEGIN_NAMESPACE

extern gbuffer::impl g_D3DBufferImpl;  // Defined in d3d_buffer.cpp

void graphics_init_buffer(gbuffer *b, graphics *g, gbuffer_type type, gbuffer_usage usage, s64 size, const char *data) {
    b->Graphics = g;
    b->Type     = type;
    b->Usage    = usage;
    b->Size     = size;

    if (g->API == graphics_api::Direct3D) {
        b->Impl = &g_D3DBufferImpl;
    } else {
        assert(false);
    }

    b->Impl->Init(b, data);
}

void set_layout(gbuffer *b, gbuffer_layout layout) { b->Impl->SetLayout(b, layout); }

void *map(gbuffer *b, gbuffer_map_access access) { return b->Impl->Map(b, access); }

void unmap(gbuffer *b) { b->Impl->Unmap(b); }

void bind_vb(gbuffer *b, primitive_topology topology, u32 offset, u32 customStride) {
    b->Impl->Bind(b, topology, offset, customStride, (shader_type) 0, 0);
}

void bind_ib(gbuffer *b, u32 offset) { b->Impl->Bind(b, (primitive_topology) 0, offset, 0, (shader_type) 0, 0); }

void bind_ub(gbuffer *b, shader_type shaderType, u32 position) {
    b->Impl->Bind(b, (primitive_topology) 0, 0, 0, shaderType, position);
}

void unbind(gbuffer *b) { b->Impl->Unbind(b); }

void free_buffer(gbuffer *b) {
    if (b->Impl) b->Impl->Free(b);
}

LSTD_END_NAMESPACE
