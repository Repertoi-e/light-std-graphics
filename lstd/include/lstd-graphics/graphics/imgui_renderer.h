#pragma once

#include "api.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "vendor/imgui/imgui.h"

struct imgui_renderer {
    graphics *Graphics = null;

    gbuffer VB, IB, UB;
    texture_2D FontTexture;
    shader Shader;
    s64 VBSize = 0, IBSize = 0;

    imgui_renderer() {}

    void init(graphics *g);
    void draw(ImDrawData *drawData);
    void release();

   private:
    void set_render_state();
};
