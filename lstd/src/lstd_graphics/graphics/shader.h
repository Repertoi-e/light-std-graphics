#pragma once

#include "asset.h"
#include "gtype.h"

#if OS == WINDOWS
struct ID3D11VertexShader;
struct ID3D11PixelShader;
#endif

LSTD_BEGIN_NAMESPACE

struct graphics;

// Holds both a vertex and a pixel shader (those are the two shader types we support for now!)
struct shader : public asset, non_copyable, non_movable {
#if OS == WINDOWS
    struct {
        ID3D11VertexShader *VS = null;
        ID3D11PixelShader *PS = null;
        void *VSBlob = null;  // We can't forward declare _ID3DBlob_
        void *PSBlob = null;
    } D3D{};
#endif

    struct impl {
        void (*Init)(shader *s) = null;
        void (*Bind)(shader *s) = null;
        void (*Unbind)(shader *s) = null;
        void (*Release)(shader *s) = null;
    } Impl{};

    graphics *Graphics;

    string Source;

    struct uniform {
        string Name;
        gtype Type = gtype::Unknown;

        s64 ByteSize = 0;
        s64 Offset = 0;
        s64 Count = 0;  // _Count_ * _Size_ gives the total size
    };

    struct uniform_buffer {
        string Name;
        s64 ByteSize = 0;
        array<uniform> Uniforms;

        shader_type ShaderType = shader_type::None;
        u32 Position = 0;
    };

    // We parse the shader source and extract metadata (not really sophisticated...)
    // !!! We actually removed parsing a long time ago because it wasn't working
    // @TODO
    array<uniform_buffer> UniformBuffers;

    shader() {}

    void init_from_file(graphics *g, string file);
    void init_from_source(graphics *g, string source);

    void bind();
    void unbind();

    void release();
};

LSTD_END_NAMESPACE
