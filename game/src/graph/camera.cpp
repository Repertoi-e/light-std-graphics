#include "state.h"

void camera_reinit(camera *cam) {
    cam->Position = v2(0, 0);
    cam->Scale = v2(50, 50);  // Good starting value (arbitrary)

    cam->Panning = false;
    cam->ScrollY = 0;

    camera_reset_constants(cam);
}

void camera_reset_constants(camera *cam) {
    // These are arbitrary, feel good though
    cam->PanSpeed = 0.05f;
    cam->ZoomSpeed = 3.0f;
    cam->ScaleMin = 5.0f;
    cam->ScaleMax = 500.0f;
}

void camera_update(camera *cam) {
    // The viewport window may not be in an additional imgui window since
    // we don't allow moving it, so assuming this is is fine.
    auto *win = GameMemory->MainWindow;

    vec2<s32> mouse = win->get_cursor_pos();
    v2 delta = {(f32) mouse.x - GameState->LastMouse.x, (f32) mouse.y - GameState->LastMouse.y};
    GameState->LastMouse = mouse;

    // This gets set when we listen for the scroll event
    if (cam->ScrollY) {
        // We map our scale range [ScaleMin, ScaleMax] to the range [1, ZoomSpeedup]
        // and then we speedup our scaling by a cubic factor.
        // (Faster zooming the more zoomed you are.)
        f32 x = 1 + 1 / (cam->ScaleMax - cam->ScaleMin) * (cam->Scale.x - cam->ScaleMin);
        f32 zoomFactor = x * x;

        f32 keyFactor = 1;
        if (win->Keys[Key_LeftControl]) {
            keyFactor = 3;
        }
        cam->Scale += cam->ZoomSpeed * cam->ScrollY * keyFactor * zoomFactor;

        if (cam->Scale.x < cam->ScaleMin) cam->Scale = v2(cam->ScaleMin, cam->ScaleMin);
        if (cam->Scale.x > cam->ScaleMax) cam->Scale = v2(cam->ScaleMax, cam->ScaleMax);

        cam->ScrollY = 0;
    }

    // Start panning if clicking inside the viewport, finish panning when the mouse is released
    if (win->MouseButtons[Mouse_Button_Left]) {
        if (mouse_in_viewport()) {
            cam->Panning = true;
        }
    } else {
        cam->Panning = false;
    }

    if (cam->Panning) {
        f32 speed = cam->PanSpeed * cam->ScaleMax / cam->Scale.x;

        v2 up = v2(0, 1);
        v2 right = v2(1, 0);

        cam->Position -= right * delta.x * speed;
        cam->Position -= up * delta.y * speed;
    }
}
