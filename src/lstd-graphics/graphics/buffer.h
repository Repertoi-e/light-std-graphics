#pragma once

#include "gtype.h"

import "lstd.h";

struct gbuffer_layout_element {
    string Name;
    gtype Type;
    s64 SizeInBits;
    bool Normalized;
    s64 Count;
};

using gbuffer_layout = array<gbuffer_layout_element>;

inline gbuffer_layout_element layout_element(string name, gtype type, s64 count = 1, bool normalized = false) {
    assert(type != gtype::Unknown);
    s64 sizeInBits = get_size_of_base_gtype_in_bits(type);

    // e.g. F32_3x2 has 6 floats
    count *= get_count_of_gtype(type);

    return {name, get_scalar_gtype(type), sizeInBits, normalized, count};
}

inline gbuffer_layout_element layout_padding(s64 bytes) { return {"", gtype::Unknown, bytes * 8, false, 1}; }

enum class primitive_topology { PointList = 0,
                                LineList,
                                LineStrip,
                                TriangleList,
                                TriangleStrip };

enum class gbuffer_type {
    None = 0,

    Vertex_Buffer,
    Index_Buffer,

    // Used to pack shader data, called "constant buffers" (DX) or "uniform buffer objects" (GL)
    Shader_Uniform_Buffer
};

// This only makes sense when using DX, GL doesn't support these options when binding a buffer
enum class gbuffer_usage {
    Default,    // The buffer requires read and write access by the GPU
    Immutable,  // Cannot be modified after creation, so it must be created with initial data
    Dynamic,    // Can be written to by the CPU and read by the GPU
    Staging     // Supports data transfer (copy) from the GPU to the CPU
};

enum class gbuffer_map_access {
    Read,        // Buffer can only be read by the CPU
    Write,       // Buffer can only be written to by the CPU
    Read_Write,  // Buffer can be read and written to by the CPU

    // Previous contents of buffer may be discarded, and new buffer is opened for writing
    Write_Discard_Previous,

    // An advanced option that allows you to add more data to the buffer even while the GPU is
    // using parts. However, you must not work with the parts the GPU is using.
    Write_Unsynchronized

};

struct graphics;

struct ID3D11Buffer;
struct ID3D11InputLayout;

struct gbuffer {
#if OS == WINDOWS
    struct {
        ID3D11Buffer *Buffer      = null;
        ID3D11InputLayout *Layout = null;

        // Based on the definition of _D3D11_MAPPED_SUBRESOURCE_,
        // we don't include DirectX headers here...
        char MappedData[POINTER_SIZE + sizeof(u32) + sizeof(u32)]{};
    } D3D{};
#endif

    struct impl {
        void (*Init)(gbuffer *b, const char *initialData);
        void (*SetLayout)(gbuffer *b, gbuffer_layout layout);

        void *(*Map)(gbuffer *b, gbuffer_map_access access);
        void (*Unmap)(gbuffer *b);

        void (*Bind)(gbuffer *b, primitive_topology topology, u32 offset, u32 stride, shader_type shaderType, u32 position);
        void (*Unbind)(gbuffer *b);
        void (*Free)(gbuffer *b);
    };
    impl *Impl = null;

    // The graphics object associated with this buffer
    graphics *Graphics = null;

    gbuffer_type Type   = gbuffer_type::None;
    gbuffer_usage Usage = gbuffer_usage::Default;

    s64 Size   = 0;
    s64 Stride = 0;  // Determined by the buffer layout
};

void graphics_init_buffer(gbuffer *b, graphics *g, gbuffer_type type, gbuffer_usage usage, s64 size, const char *initialData = null);

inline gbuffer graphics_create_buffer(graphics *g, gbuffer_type type, gbuffer_usage usage, s64 size, const char *initialData = null) {
    gbuffer b;
    graphics_init_buffer(&b, g, type, usage, size, initialData);
    return b;
}

void set_layout(gbuffer *b, array<gbuffer_layout_element> layout);

void *map(gbuffer *b, gbuffer_map_access access);
void unmap(gbuffer *b);

void bind_vb(gbuffer *b, primitive_topology topology, u32 offset = 0, u32 customStride = 0);
void bind_ib(gbuffer *b, u32 offset = 0);
void bind_ub(gbuffer *b, shader_type shaderType, u32 position);

void unbind(gbuffer *b);

void free_buffer(gbuffer *b);
