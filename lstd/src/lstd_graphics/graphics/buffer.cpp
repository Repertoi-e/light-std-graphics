#include "buffer.h"

#include "api.h"

LSTD_BEGIN_NAMESPACE

extern buffer::impl g_D3DBufferImpl;  // Defined in d3d_buffer.cpp

void graphics_init_buffer(gbuffer *b, graphics *g, buffer_type type, buffer_usage usage, s64 size, const char *data) {
    b->Graphics = g;
    b->Type     = type;
    b->Usage    = usage;
    b->Size     = size;

    if (g->API == graphics_api::Direct3D) {
        b->Impl = &g_D3DBufferImpl;
    } else {
        assert(false);
    }

    b->Impl->Init(this, data);
}

void set_layout(gbuffer *b, array<buffer_layout_element> layout) { Impl->SetLayout(this, layout); }

void *map(gbuffer *b, buffer_map_access access) { return b->Impl->Map(this, access); }

void unmap(gbuffer *b, ) { b->Impl->Unmap(this); }

void bind_vb(gbuffer *b, primitive_topology topology, u32 offset, u32 customStride) {
    Impl.Bind(this, topology, offset, customStride, (shader_type) 0, 0);
}

void bind_ib(gbuffer *b, u32 offset) { Impl.Bind(this, (primitive_topology) 0, offset, 0, (shader_type) 0, 0); }

void bind_ub(gbuffer *b, shader_type shaderType, u32 position) {
    Impl.Bind(this, (primitive_topology) 0, 0, 0, shaderType, position);
}

void unbind(gbuffer *b) { Impl.Unbind(this); }

void free_buffer(gbuffer *b, gbuffer *b) {
    if (b->Impl) b->Impl->Free(this);
}

LSTD_END_NAMESPACE
