#include "state.h"

void camera_reinit(camera *cam) {
    cam->Position = v2(0, 0);
    cam->Scale = v2(50, 50); // Good starting value (arbitrary)
    camera_reset_constants(cam);
}

void camera_reset_constants(camera *cam) {
    cam->PanSpeed = 0.15f;
    cam->ZoomSpeed = 0.3f;
    cam->ZoomMin = 5.0f;
    cam->ZoomMax = 500.0f;
}

void camera_update(camera *cam) {
    // The viewport window may not be in an additional imgui window since
    // we don't allow moving it, so assuming this is is fine.
    auto *win = GameMemory->MainWindow;

    static vec2<s32> lastMouse = zero();

    vec2<s32> mouse = win->get_cursor_pos();
    v2 delta = {(f32) mouse.x - lastMouse.x, (f32) mouse.y - lastMouse.y};
    lastMouse = mouse;

    if (win->Keys[Key_LeftControl]) {
        if (win->MouseButtons[Mouse_Button_Left]) {
            f32 speed = cam->PanSpeed * cam->ZoomMax / cam->Scale.x;

            v2 up = v2(0, 1);
            v2 right = v2(1, 0);

            cam->Position -= right * delta.x * speed;
            cam->Position -= up * delta.y * speed;
        } else if (win->MouseButtons[Mouse_Button_Right]) {
            // We map our scale range [ZoomMin, ZoomMax] to the range [1, ZoomSpeedup]
            // and then we speedup our scaling by a cubic factor.
            // (Faster zooming the more zoomed you are.)
            f32 x = 1 + 1 / (cam->ZoomMax - cam->ZoomMin) * (cam->Scale.x - cam->ZoomMin);
            f32 speed = x * x * x;
            cam->Scale += delta.y * cam->ZoomSpeed * speed;

            if (cam->Scale.x < cam->ZoomMin) cam->Scale = v2(cam->ZoomMin, cam->ZoomMin);
            if (cam->Scale.x > cam->ZoomMax) cam->Scale = v2(cam->ZoomMax, cam->ZoomMax);
        }
    }
}
