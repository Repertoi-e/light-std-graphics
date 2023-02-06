#include <driver.h>
#include "state.h"

void ui_main() {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("CDock Window", null, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("CDock");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            bool vsync = Memory->MainWindow.GetFlags() & window_flags::VSYNC;
            if (ImGui::MenuItem("VSync", "", vsync)) {
                Memory->MainWindow.SetVsync(!vsync);
            }

            if (ImGui::MenuItem("Display AST", "", GraphState->DisplayAST)) {
                GraphState->DisplayAST = !GraphState->DisplayAST;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Used for debugging problems with the expression parser.");
                ImGui::TextUnformatted("");
                ImGui::TextUnformatted("When enabled, displays the Abstract Syntax Tree below the expression input field.");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::EndMenu();
        }
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("This is an awesome calculator written entirely (expect ImGui) from scratch in order to battle-test my C++ standard library replacement.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("* Camera controls");
            ImGui::TextUnformatted("      Left Mouse -> Pan");
            ImGui::TextUnformatted("      Scroll -> Zoom");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("This project is under the MIT license.");
            ImGui::TextUnformatted("Source code: github.com/Repertoi-e/light-std-graphics/");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void ui_scene_properties() {
    auto *cam = &GraphState->Camera;

    ImGui::Begin("Scene", null);
    {
        if (ImGui::Button("Reset camera")) camera_reinit(cam);

        ImGui::Text("Position: %.3f, %.3f", cam->Position.x, cam->Position.y);
        ImGui::Text("Scale (zoom): %.3f, %.3f", cam->Scale.x, cam->Scale.y);

        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Pan speed", &cam->PanSpeed);
        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Zoom speed", &cam->ZoomSpeed);
        ImGui::InputFloat2("Scale min/max", &cam->ScaleMin);
        if (ImGui::Button("Default camera constants")) camera_reset_constants(cam);
    }
    {
        ImGui::Text("Frame information:");
        ImGui::Text("  %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Clear color:");
        ImGui::ColorPicker3("", &GraphState->ClearColor.x, ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Reset color")) GraphState->ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};
    }
    ImGui::End();
}

string validate_and_parse_formula(function_entry *f) {
    // This function returns an error message (if there was one!).

    auto s = string(f->Formula);
    auto it = search(s, '{');

    string range;
    if (it != -1) {
        range = slice(s, it + 1, length(s));
        if (range.Count && range[-1] != '}') {
            return "Expected } for end of range";
        }
        if (range.Count) range = slice(range, 0, -1);

        s     = slice(s, 0, it);
    }

    token_stream tokens;
    PUSH_ALLOC(TemporaryAllocator) {
        tokens = tokenize(s);
        if (tokens.Error.Count) {
            return tokens.Error;
        }

        validate_expression(&tokens);
        if (tokens.Error.Count) {
            return tokens.Error;
        }
    }

    tokens.It    = tokens.Tokens;  // Reset it
    tokens.Error = "";

    ast *root = parse_expression(&tokens);
    assert(!tokens.Error.Count);  // We should've caught that when validating, no?

    // Store the AST
    f->FormulaRoot = root;
    f->HasRange = range.Count;

    if (range.Count) {
        token_stream rangeTokens;
        PUSH_ALLOC(TemporaryAllocator) {
            rangeTokens = tokenize(range);
            if (rangeTokens.Error.Count) {
                return rangeTokens.Error;
            }

            auto t = rangeTokens.Tokens;
            if (t.Count != 2 || t[0].Type != token::NUMBER || t[1].Type != token::NUMBER) {
                return "Invalid range - specify two numbers separated by a space";
            }

            f->Begin = t[0].F64Value;
            f->End   = t[1].F64Value;

            if (f->End <= f->Begin) {
                return "Invalid range - second number should be larger";
            }
        }
    }

    return "";
}

void display_ast(ast *node) {
    if (!node) return;

    if (node->Type == ast::OP) {
        // We use a unique node id because otherwise nodes with the same titles share properties (just the way ImGui works)
        auto *nodeTitle = mprint("OP {:c}##{}", ((ast_op *) node)->Op, node->ImGuiID);

        if (ImGui::TreeNode(nodeTitle)) {
            display_ast(node->Left);
            display_ast(node->Right);

            ImGui::TreePop();
        }
    } else if (node->Type == ast::TERM) {
        auto *var = (ast_term *) node;

        string_builder b;
        string_builder_writer w;
        w.Builder = &b;

        PUSH_ALLOC(TemporaryAllocator) {
            fmt_to_writer(&w, "{:g} ", var->Coeff);
            for (auto [k, v] : var->Letters) {
                fmt_to_writer(&w, "{:c}^{} ", *k, *v);
            }
        }

        auto *nodeTitle = mprint("TERM {}##{}", b, node->ImGuiID);
        if (ImGui::TreeNode(nodeTitle)) {
            ImGui::TreePop();
        }
    }
}

void determine_new_parameters(function_entry *f, hash_table<code_point, f64> oldParams, ast *node) {
    if (!node) return;

    determine_new_parameters(f, oldParams, node->Left);
    determine_new_parameters(f, oldParams, node->Right);

    if (node->Type == ast::TERM) {
        for (auto [k, _] : ((ast_term *) node)->Letters) {
            if (*k != 'x') {  // @TODO
                auto [kf, vf] = search(oldParams, *k);
                if (kf && vf) {
                    set(f->Parameters, *k, *vf);
                } else {
					add(f->Parameters, *k, 0);
                }
            }
        }
    }
}

void ui_functions() {
    ImGui::Begin("Functions", null);
    {
        s64 indexToRemove = -1;

        // @Cleamup for_enumerate doesn't work for some reason
        For_as(it_index, range(GraphState->Functions.Count)) {
            auto *it = GraphState->Functions.Data + it_index;

            ImGui::PushID(it->ImGuiID);

            ImGui::ColorEdit3("", &it->Color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);

            ImGui::SameLine();
            if (ImGui::InputText("", it->Formula, function_entry::FORMULA_INPUT_BUFFER_SIZE)) {
                if (it->FormulaRoot) {
                    free_ast(it->FormulaRoot);
                    it->FormulaRoot = null;
                }

                string error = validate_and_parse_formula(it);
                it->FormulaMessage = clone(error);

                if (!it->FormulaMessage.Count) {
                    hash_table<code_point, f64> oldParams;
                    PUSH_ALLOC(TemporaryAllocator) {
                        oldParams = clone(it->Parameters);
                    }
                    free(it->Parameters);
                    determine_new_parameters(it, oldParams, it->FormulaRoot);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("X")) {
                indexToRemove = it_index;
            }

            if (it->FormulaMessage.Count) {
                ImGui::Text(to_c_string_temp(it->FormulaMessage));
            } else if (GraphState->DisplayAST) {
                display_ast(it->FormulaRoot);
            }

            for (auto [k, v] : it->Parameters) {
                f64 min = -30, max = 30;
                ImGui::SliderScalar(mprint("{:c}", *k), ImGuiDataType_Double, v, &min, &max, "%.7f", ImGuiSliderFlags_NoRoundToFormat);
            }

            ImGui::PopID();
        }

        if (indexToRemove != -1) {
            free_function_entry(GraphState->Functions.Data + indexToRemove);
            remove_ordered_at_index(GraphState->Functions, indexToRemove);
        }

        if (ImGui::Button("Add entry")) {
            add(GraphState->Functions, function_entry{});
        }
    }
    ImGui::End();
}