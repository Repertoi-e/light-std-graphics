#include "imgui_renderer.h"

#include "../graphics/bitmap.h"
#include "../video/window.h"

void imgui_renderer::init(graphics *g) {
    assert(!Graphics);
    Graphics = g;

    ImGuiIO &io            = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_lstd_graphics";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGuiPlatformIO &platformIO      = ImGui::GetPlatformIO();
    platformIO.Renderer_RenderWindow = [](auto *viewport, void *context) {
        window win;
        win.ID = (u32) (u64) viewport->PlatformHandle;
        if (!win.is_visible()) return;

        auto *renderer = (imgui_renderer *) context;
        if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
            renderer->Graphics->clear_color(float4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        renderer->draw(viewport->DrawData);
    };

    Shader.Name = "UI Shader";
    Shader.init_from_file(g, "data/UI.hlsl");

    graphics_init_buffer(&UB, g, gbuffer_type::Shader_Uniform_Buffer, gbuffer_usage::Dynamic, sizeof(mat<f32, 4, 4>));

    s32 width, height;
    u8 *pixels = null;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FontTexture.init(g, width, height);
    FontTexture.set_data(make_bitmap(pixels, width, height, pixel_format::RGBA));

    io.Fonts->TexID = &FontTexture;
}

void imgui_renderer::draw(ImDrawData *drawData) {
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) return;

    if (VBSize <= drawData->TotalVtxCount) {
        free_buffer(&VB);

        VBSize = drawData->TotalVtxCount + 5000;
        graphics_init_buffer(&VB, Graphics, gbuffer_type::Vertex_Buffer, gbuffer_usage::Dynamic, VBSize * sizeof(ImDrawVert));

        Shader.bind();

        gbuffer_layout layout;
        make_dynamic(&layout, 3);
        add(&layout, layout_element("POSITION", gtype::F32_2));
        add(&layout, layout_element("TEXCOORD", gtype::F32_2));
        add(&layout, layout_element("COLOR", gtype::U32, 1, true));
        set_layout(&VB, layout);

        free(layout.Data);
    }

    if (IBSize <= drawData->TotalIdxCount) {
        IBSize = drawData->TotalIdxCount + 10000;
        graphics_init_buffer(&IB, Graphics, gbuffer_type::Index_Buffer, gbuffer_usage::Dynamic, IBSize * sizeof(u32));
    }

    auto *vb = (ImDrawVert *) map(&VB, gbuffer_map_access::Write_Discard_Previous);
    auto *ib = (u32 *) map(&IB, gbuffer_map_access::Write_Discard_Previous);

    For_as(it_index, range(drawData->CmdListsCount)) {
        auto *it = drawData->CmdLists[it_index];
        copy_memory(vb, it->VtxBuffer.Data, it->VtxBuffer.Size * sizeof(ImDrawVert));
        copy_memory(ib, it->IdxBuffer.Data, it->IdxBuffer.Size * sizeof(u32));
        vb += it->VtxBuffer.Size;
        ib += it->IdxBuffer.Size;
    }
    unmap(&VB);
    unmap(&IB);

    auto *ub      = map(&UB, gbuffer_map_access::Write_Discard_Previous);
    f32 L         = drawData->DisplayPos.x;
    f32 R         = drawData->DisplayPos.x + drawData->DisplaySize.x;
    f32 T         = drawData->DisplayPos.y;
    f32 B         = drawData->DisplayPos.y + drawData->DisplaySize.y;
    f32 mvp[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, 0.5f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
    };
    copy_memory_fast(ub, &mvp, sizeof(mvp));
    unmap(&UB);

    set_render_state();

    rect oldScissorRect = Graphics->get_scissor_rect();

    s32 vtxOffset = 0, idxOffset = 0;
    For_as(cmdListIndex, range(drawData->CmdListsCount)) {
        auto *cmdList = drawData->CmdLists[cmdListIndex];
        For(cmdList->CmdBuffer) {
            if (it.UserCallback) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer
                // to reset render state.)
                if (it.UserCallback == ImDrawCallback_ResetRenderState) {
                    set_render_state();
                } else {
                    it.UserCallback(cmdList, &it);
                }
            } else {
                s32 left  = (s32) (it.ClipRect.x - drawData->DisplayPos.x);
                s32 top   = (s32) (it.ClipRect.y - drawData->DisplayPos.y);
                s32 right = (s32) (it.ClipRect.z - drawData->DisplayPos.x);
                s32 bot   = (s32) (it.ClipRect.w - drawData->DisplayPos.y);

                rect r;
                r.top    = top;
                r.left   = left;
                r.bottom = bot;
                r.right  = right;
                Graphics->set_scissor_rect(r);

                if (it.TextureId) ((texture_2D *) it.TextureId)->bind(0);
                Graphics->draw_indexed(it.ElemCount, it.IdxOffset + idxOffset, it.VtxOffset + vtxOffset);
            }
        }
        idxOffset += cmdList->IdxBuffer.Size;
        vtxOffset += cmdList->VtxBuffer.Size;
    }
    Graphics->set_scissor_rect(oldScissorRect);
}

void imgui_renderer::release() {
    free_buffer(&VB);
    free_buffer(&IB);
    free_buffer(&UB);
    ImGui::GetIO().Fonts->TexID = null;
    FontTexture.release();
    Shader.release();
}

void imgui_renderer::set_render_state() {
    Shader.bind();
    bind_vb(&VB, primitive_topology::TriangleList);
    bind_ib(&IB);
    bind_ub(&UB, shader_type::Vertex_Shader, 0);
}
