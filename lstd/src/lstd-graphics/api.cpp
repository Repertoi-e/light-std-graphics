#include "lstd-graphics/graphics/api.h"

#include "lstd-graphics/video/window.h"
#include "lstd-graphics/graphics/buffer.h"
#include "lstd-graphics/graphics/shader.h"
#include "lstd-graphics/graphics/texture.h"

extern graphics::impl g_D3DImpl;  // Defined in d3d_api.cpp
extern graphics::impl g_GLImpl;  // Defined in gl_api.cpp

void graphics::init(graphics_api api) {
    API = api;
    if (api == graphics_api::Direct3D) {
        Impl = g_D3DImpl;
    } else if (api == graphics_api::OpenGL) {
        Impl = g_GLImpl;
    } else {
        assert(false);
    }
    Impl.Init(this);

    reserve(TargetWindows, 8);
search(TargetWindows, )
    auto predicate = [](auto x) { return !x.Window; };
    if (find(TargetWindows, &predicate) == -1) add(TargetWindows, target_window{});  // Add a null target
    set_target_window({});
}

// Sets the current render context, so you can draw to multiple windows using the same _graphics_ object.
// If you want to draw to a texture, use _set_custom_render_target_, note that you must still have a valid
// target window, and that window is associated with the resources which get created.

void graphics::set_target_window(window win) {
    auto predicate = [&](auto x) { return x.Window == win; };
    s64 index      = find(TargetWindows, &predicate);

    target_window *targetWindow;
    if (index == -1) {
        targetWindow         = add(TargetWindows, target_window{});
        targetWindow->Window = win;
        if (win) {
            targetWindow->CallbackID = connect_event(win, {this, &graphics::window_event_handler});
            Impl.InitTargetWindow(this, targetWindow);

            event e;
            e.Window = win;
            e.Type   = event::Window_Resized;

            int2 s = get_size(win);
            e.Width  = s.x;
            e.Height = s.y;
            window_event_handler(e);
        }
    } else {
        targetWindow = TargetWindows.Data + index;
    }

    CurrentTargetWindow = targetWindow;
    if (win) set_custom_render_target(targetWindow->CustomRenderTarget);
}

rect graphics::get_viewport() {
    assert(CurrentTargetWindow->Window);
    return CurrentTargetWindow->Viewport;
}

void graphics::set_viewport(rect viewport) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->Viewport = viewport;
    Impl.SetViewport(this, viewport);
}

rect graphics::get_scissor_rect() {
    assert(CurrentTargetWindow->Window);
    return CurrentTargetWindow->ScissorRect;
}

void graphics::set_scissor_rect(rect scissorRect) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->ScissorRect = scissorRect;
    Impl.SetScissorRect(this, scissorRect);
}

// Pass _null_ to restore rendering to the back buffer

void graphics::set_custom_render_target(texture_2D *target) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->CustomRenderTarget = target;
    Impl.SetRenderTarget(this, target);

    set_cull_mode(CurrentTargetWindow->CullMode);

    int2 size = get_size(CurrentTargetWindow->Window);
    if (target) size = {target->Width, target->Height};

    rect r;
    r.top = 0;
    r.left = 0;
    r.bottom = size.y;
    r.right  = size.x;
    set_viewport(r);
    set_scissor_rect(r);
}

void graphics::set_blend(bool enabled) { Impl.SetBlend(this, enabled); }

void graphics::set_depth_testing(bool enabled) { Impl.SetDepthTesting(this, enabled); }

void graphics::set_cull_mode(cull mode) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->CullMode = mode;
    Impl.SetCullMode(this, mode);
}

void graphics::clear_color(float4 color) {
    assert(CurrentTargetWindow->Window);
    if (!is_visible(CurrentTargetWindow->Window)) return;

    Impl.ClearColor(this, color);
}

void graphics::draw(u32 vertices, u32 startVertexLocation) { Impl.Draw(this, vertices, startVertexLocation); }

void graphics::draw_indexed(u32 indices, u32 startIndex, u32 baseVertexLocation) {
    Impl.DrawIndexed(this, indices, startIndex, baseVertexLocation);
}

void graphics::swap() {
    assert(CurrentTargetWindow->Window);
    if (!is_visible(CurrentTargetWindow->Window)) return;

    Impl.Swap(this);
}

bool graphics::window_event_handler(const event &e) {
    if (e.Type == event::Window_Closed) {
        auto predicate = [&](auto x) { return x.Window == e.Window; };
        s64 index      = find(TargetWindows, &predicate);
        assert(index != -1);

        target_window *targetWindow = TargetWindows.Data + index;
        disconnect_event(targetWindow->Window, targetWindow->CallbackID);
        Impl.ReleaseTargetWindow(this, targetWindow);

        remove_unordered_at_index(&TargetWindows, index);
    } else if (e.Type == event::Window_Resized) {
        auto predicate = [&](auto x) { return x.Window == e.Window; };
        s64 index      = find(TargetWindows, &predicate);
        assert(index != -1);

        if (!is_visible(e.Window)) return false;
        Impl.TargetWindowResized(this, TargetWindows.Data + index, e.Width, e.Height);
    }
    return false;
}

void graphics::free() {
    if (Impl.Release) {
        Impl.Release(this);
        API = graphics_api::None;
    }

    For(TargetWindows) {
        if (it.Window) {
            disconnect_event(it.Window, it.CallbackID);
            Impl.ReleaseTargetWindow(this, &it);
        }
    }
    ::free(TargetWindows.Data);
}
