#include "state.h"

import fmt;

f32 graph_function(f32 x) {
    f32 a = GameState->QuadraticA;
    f32 b = GameState->QuadraticB;
    f32 c = GameState->QuadraticC;

    // return a * x * x + b * x + c;
    return Math_ArcTan_flt32(x);
}

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

        // Any vertices added in this block get treated as world space (or "graph" space) and then get transformed into screen space
        {
            auto invMat = GameState->InverseViewMatrix;

            // Here we get the min max bounding box of the graph (which may be transformed by the camera).
            // We use this to prevent drawing anything outside the visible space.
            //
            // +- (100, 100) [pixels] so we draw just a bit outside of the screen, for safety.
            //
            v2 topLeft = dot(viewportPos - v2(100, 100), invMat);
            
            // Remember that y is flipped compared to the normal convention in graphics, bottom of the screen here means +y
            v2 bottomRight = dot(viewportPos + viewportSize + v2(100, 100), invMat);

            f32 xmin = topLeft.x, xmax = bottomRight.x;
            f32 ymin = topLeft.y, ymax = bottomRight.y;

            // d->AddLine(topRight + v2(10, 10), bottomLeft - v2(10, 10), 0xff00ff00, 2);
            f32 ppm = GameState->PixelsPerMeter;

            f32 thickness = 1;

            // Choose just one value since thickness is 1D.
            f32 thicknessScaleFactor = 1 / max(GameState->Camera.Scale);
            thickness *= thicknessScaleFactor;
             
            // v2 step = v2(1, 1); // How much we move between each line (in terms of x) 
               
            // Scale in order to fit around 30 squares
            v2 step = (bottomRight - topLeft) / (10 * ppm);
            
            // Take just one scale (otherwise we might get rectangles in the guide lines) 
            f32 s = min(step);

            // Round to the nearest fraction in the form n/5, e.g. 1/20, 1/15, 1/10, 1/5, 1, 5, 10, etc..
            bool flipped = false;
            if (s < 1) {
                s = 1 / s;
                flipped = true;
            }
            
            // This will be always positive,
            s = Math_RoundDown_flt32(s / 5);
            // so this will be always be > 5 
            s = (s + 1) * 5;

            if (flipped) {
                s = 1 / (s - 5);
            }

            step = v2(s);

            step *= ppm;

            v2 firstLine = topLeft - v2(ImFmod(topLeft.x, step.x), ImFmod(topLeft.y, step.y));
            
            v2 p = firstLine; // Stores the position of the next line to draw
            
            auto steps = vec2<s64>((bottomRight - topLeft) / step);
            For(range((s64)steps.x)) {
                d->AddLine(v2(p.x, ymin), v2(p.x, ymax), 0xddcfcfcf, thickness);
                p.x += step.x;
            }

            For(range((s64)steps.y)) {
                d->AddLine(v2(xmin, p.y), v2(xmax, p.y), 0xddcfcfcf, thickness);
                p.y += step.y;
            }

            d->AddLine(v2(0, ymin), v2(0, ymax), 0xddebb609, thickness * 2);
            d->AddLine(v2(xmin, 0), v2(xmax, 0), 0xddebb609, thickness * 2);

            // f32 xmin = -range / ppm;
            // f32 xmax = range / ppm;
            //
            // f32 xstep = 0.1f;
            //
            // f32 x1 = xmin - stepUnscaled;
            // f32 x2 = xmin;
            //
            // while (x1 < xmax) {
            //     f32 y1 = graph_function(x1);
            //     f32 y2 = graph_function(x2);
            //
            //     d->AddLine(v2(x1, -y1) * ppm, v2(x2, -y2) * ppm, 0xff0000ff, thickness * 3);
            //
            //     x1 = x2;
            //     x2 += xstep;
            // }
        }

        auto endVertex = d->VtxBuffer.Size;

        //
        // We now build our view transformation matrix
        //

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
