#include "state.h"

f64 evaluate_function_at(f64 x, function_entry *f, ast *node) {
    assert(node && "We shouldn't get here?");

    if (node->Type == ast::OP) {
        auto *op = (ast_op *) node;
        if (!node->Right) {  // If unary
            if (op->Op == '-') {
                return -evaluate_function_at(x, f, node->Left);
            } else {
                assert(op->Op == '+');
                // We shouldn't even generate + unary operator, but for completeness.
                return evaluate_function_at(x, f, node->Left);
            }
        } else {
            if (op->Op == '+') return evaluate_function_at(x, f, node->Left) + evaluate_function_at(x, f, node->Right);
            if (op->Op == '-') return evaluate_function_at(x, f, node->Left) - evaluate_function_at(x, f, node->Right);
            if (op->Op == '*') return evaluate_function_at(x, f, node->Left) * evaluate_function_at(x, f, node->Right);
            if (op->Op == '/') return evaluate_function_at(x, f, node->Left) / evaluate_function_at(x, f, node->Right);
            if (op->Op == '^') return Math_ExpB_flt64(evaluate_function_at(x, f, node->Left), evaluate_function_at(x, f, node->Right));
            assert(false && "Unknown operator");
            return 0.0;
        }
    } else if (node->Type == ast::TERM) {
        auto *t = (ast_term *) node;

        f64 result = t->Coeff;
        for (auto [k, exp] : t->Letters) {
            if (has(f->Parameters, *k)) {
                result += Math_ExpB_flt64(*f->Parameters[*k], *exp);
            } else {
                // @TODO
                if (*k == 'x') {
                    result += Math_ExpB_flt64(x, *exp);
                } else {
                    assert(false && "Letter not found in parameter list");
                }
            }
        }
        return result;
    } else {
        assert(false && "Unknown AST node");
        return 0.0;
    }
}

void render_viewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Graph", null, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleVar(1);

    v2 viewportPos = ImGui::GetWindowPos();
    v2 viewportSize = ImGui::GetWindowSize();
    GraphState->ViewportPos = viewportPos;
    GraphState->ViewportSize = viewportSize;
    {
        auto *d = ImGui::GetWindowDrawList();

        // Add a colored rectangle which serves as a background
        d->AddRectFilled(viewportPos, viewportPos + viewportSize, ImGui::ColorConvertFloat4ToU32(GraphState->ClearColor));

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

        v2 origin = viewportCenter - GraphState->Camera.Position;  // Camera transform

        f32 xmin = topLeft.x, xmax = bottomRight.x;
        f32 ymin = topLeft.y, ymax = bottomRight.y;

        f32 thickness = 1;

        v2 step = v2(1, 1);

        step *= GraphState->Camera.Scale;  // Camera transform

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
        For(GraphState->Functions) {
            if (!it.FormulaRoot) continue;

            f64 x0 = origin.x - step.x;
            f64 x1 = origin.x;

            while (x0 < xmax) {
                f64 y0 = evaluate_function_at(x0, &it, it.FormulaRoot);
                f64 y1 = evaluate_function_at(x1, &it, it.FormulaRoot);

                d->AddLine(v2(x0, y0), v2(x1, y1), 0xffebb609, thickness * 2.5f);

                x0 = x1;
                x1 += step.x;
            }
        }
    }
    ImGui::End();
}
