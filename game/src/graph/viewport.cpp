#include "state.h"

void viewport_render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Graph", null, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleVar(1);

    v2 viewportPos = ImGui::GetWindowPos();
    v2 viewportSize = ImGui::GetWindowSize();
    GameState->ViewportPos = viewportPos;
    GameState->ViewportSize = viewportSize;
    {
        auto *d = GameState->ViewportDrawlist = ImGui::GetWindowDrawList();

        // Add a colored rectangle which serves as a background
        d->AddRectFilled(viewportPos, viewportPos + viewportSize, ImGui::ColorConvertFloat4ToU32(GameState->ClearColor));

        auto startVertex = d->VtxBuffer.Size;
        {
            f32 ppm = GameState->PixelsPerMeter;
            
            f32 stepUnscaled = 1;
            f32 step = stepUnscaled * ppm;

            f32 thickness = 1;

            v2 p = v2(0, 0);

            f32 range = 3 * min(abs(viewportSize.x), abs(viewportSize.y));

            s64 steps = (s64)(range / step);
            for (s64 i = 0; i < steps; ++i) {
                d->AddLine(v2(p.x, -100000), v2(p.x, 100000), 0xddcfcfcf, thickness);
                d->AddLine(v2(-p.x, -100000), v2(-p.x, 100000), 0xddcfcfcf, thickness);

                d->AddLine(v2(-100000, p.y), v2(100000, p.y), 0xddcfcfcf, thickness);
                d->AddLine(v2(-100000, -p.y), v2(100000, -p.y), 0xddcfcfcf, thickness);

                p += step;
            }

            f32 a = GameState->QuadraticA;
            f32 b = GameState->QuadraticB;
            f32 c = GameState->QuadraticC;

            f32 xmin = -range / ppm;
            f32 xmax = range / ppm;

            f32 xstep = 0.1f;

            f32 x1 = xmin - stepUnscaled;
            f32 x2 = xmin;

            while (x1 < xmax) {
                f32 y1 = a * x1 * x1 + b * x1 + c;
                f32 y2 = a * x2 * x2 + b * x2 + c;

                d->AddLine(v2(x1, -y1) * ppm, v2(x2, -y2) * ppm, 0xff0000ff, thickness * 3);

                x1 = x2;
                x2 += xstep;
            }

            d->AddLine(v2(0, -100000), v2(0, 100000), 0xddebb609, thickness * 2);
            d->AddLine(v2(-100000, 0), v2(100000, 0), 0xddebb609, thickness * 2);
        }

        auto endVertex = d->VtxBuffer.Size;

        // We now build our view transformation matrix
        auto *cam = &GameState->Camera;

        // We scale and rotate based on the screen center
        m33 t = translation(viewportSize / 2 + cam->Position);
        m33 it = inverse(t);

        auto scaleRotate = dot(it, (m33) scale(cam->Scale));
        scaleRotate = dot(scaleRotate, (m33) rotation_z(-cam->Roll));
        scaleRotate = dot(scaleRotate, t);

        // Move origin to the top left of the viewport, by default it's in the top left of the whole application window.
        auto translate = dot((m33) translation(viewportPos + viewportSize / 2), (m33) translation(-cam->Position));

        // This gets blurry, we need to scale before passing to ImGui
        // auto ppmScale = (m33) scale(v2(ppm));

        GameState->ViewMatrix = dot(scaleRotate, translate);
        GameState->InverseViewMatrix = inverse(GameState->ViewMatrix);

        auto *p = d->VtxBuffer.Data + startVertex;
        For(range(startVertex, endVertex)) {
            p->pos = dot((v2) p->pos, GameState->ViewMatrix);  // Apply the view matrix to each vertex
            ++p;
        }
    }
    ImGui::End();
}
