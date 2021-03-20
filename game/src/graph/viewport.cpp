#include "state.h"

import fmt;

f32 graph_function(f32 x) {
    return Math_ArcTan_flt32(x);
}

void viewport_render() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Graph", null, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleVar(1);

    v2 viewportPos = ImGui::GetWindowPos();
    v2 viewportSize = ImGui::GetWindowSize();
    {
        auto *d = ImGui::GetWindowDrawList();

        // Add a colored rectangle which serves as a background
        d->AddRectFilled(viewportPos, viewportPos + viewportSize, ImGui::ColorConvertFloat4ToU32(GameState->ClearColor));

        // Here we get the min max bounding box of the graph (which may be transformed by the camera).
        // We use this to prevent drawing anything outside the visible space.
        //
        // +- (100, 100) [pixels] so we draw just a bit outside of the screen, for safety.
        //
        v2 topLeft = viewportPos - v2(100, 100);
        v2 bottomRight = viewportPos + viewportSize + v2(100, 100);

        // This means that 0,0 in graph space is the center of the viewport,
        // this offsets our graph space and viewport space (which may introduce unnecessary complications, but solves
        // the problem of the origin moving when resizing the window, which doesn't look cosmetically pleasing).
        v2 viewportCenter = viewportPos + viewportSize / 2;

        v2 origin = viewportCenter - GameState->Camera.Position;  // Camera transform

        f32 xmin = topLeft.x, xmax = bottomRight.x;
        f32 ymin = topLeft.y, ymax = bottomRight.y;

        f32 thickness = 1;

        // Scale the step size in order to fit around 10 squares
        v2 step = v2(1, 1);  // (bottomRight - topLeft) / 10;

        // Take just one scale (otherwise we might get rectangles in the guide lines)
        // f32 s = min(step);
        //
        // // Round to the nearest fraction in the form n/5, e.g. 1/20, 1/15, 1/10, 1/5, 1, 5, 10, etc..
        // bool flipped = false;
        // if (s < 1) {
        //     s = 1 / s;
        //     flipped = true;
        // }
        //
        // // This will be always positive,
        // s = Math_RoundDown_flt32(s / 5);
        // // so this will be always be > 5
        // s = (s + 1) * 5;
        //
        // if (flipped) {
        //     s = 1 / (s - 5);
        // }
        //
        // step = v2(s);

        step *= GameState->Camera.Scale;  // Camera transform

        v2 steps = (bottomRight - topLeft) / step;

        v2 offset = topLeft - origin;
        offset.x = ImFmod(offset.x, step.x);  // @Cleanup
        offset.y = ImFmod(offset.y, step.y);  // @Cleanup
        
        v2 firstLine = topLeft - offset;  

        // Draw lines
        v2 p = firstLine;  // Stores the position of the next line to draw
        For(range((s64) steps.x)) {
            d->AddLine(v2(p.x, ymin), v2(p.x, ymax), 0xddcfcfcf, thickness);
            p.x += step.x;
        }

        For(range((s64) steps.y)) {
            d->AddLine(v2(xmin, p.y), v2(xmax, p.y), 0xddcfcfcf, thickness);
            p.y += step.y;
        }

        // Draw origin lines
        d->AddLine(v2(origin.x, ymin), v2(origin.x, ymax), 0xddebb609, thickness * 2);
        d->AddLine(v2(xmin, origin.y), v2(xmax, origin.y), 0xddebb609, thickness * 2);

        // Draw function graph
    }
    ImGui::End();
}
