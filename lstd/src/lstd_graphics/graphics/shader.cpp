#include "shader.h"

#include "api.h"

import lstd.path;

LSTD_BEGIN_NAMESPACE

extern shader::impl g_D3DShaderImpl;  // Defined in d3d_shader.cpp
void shader::init_from_file(graphics *g, string file) {
    clone(&FilePath, file);

    auto [source, sucess] = path_read_entire_file(file);
    if (!sucess) return;

    Graphics = g;
    Source = source;

    if (g->API == graphics_api::Direct3D) {
        Impl = g_D3DShaderImpl;
    } else {
        assert(false);
    }
    Impl.Init(this);
}

void shader::init_from_source(graphics *g, string source) {
    Graphics = g;

    clone(&Source, source);

    if (g->API == graphics_api::Direct3D) {
        Impl = g_D3DShaderImpl;
    } else {
        assert(false);
    }
    Impl.Init(this);
}

void shader::bind() { Impl.Bind(this); }
void shader::unbind() { Impl.Unbind(this); }

void shader::release() {
    if (Impl.Release) Impl.Release(this);
}

LSTD_END_NAMESPACE
