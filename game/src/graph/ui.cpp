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
            if (ImGui::MenuItem("VSync", "", GameMemory->MainWindow->Flags & window::VSYNC))
                GameMemory->MainWindow->Flags ^= window::VSYNC;
            if (ImGui::MenuItem("Display AST", "", GraphState->DisplayAST))
                GraphState->DisplayAST = !GraphState->DisplayAST;

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

    token_stream tokens;

    WITH_ALLOC(Context.Temp) {
        tokens = tokenize(string(f->Formula));
        if (tokens.Error) {
            return tokens.Error;
        }

        validate_expression(tokens);
        if (tokens.Error) {
            return tokens.Error;
        }
    }

    tokens.It = tokens.Tokens;  // Reset it
    tokens.Error = "";

    ast *root = parse_expression(tokens);
    assert(!tokens.Error);  // We should've caught that when validating, no?

    // Store the AST
    f->FormulaRoot = root;

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

        string_builder_writer w;
        WITH_ALLOC(Context.Temp) {
            fmt_to_writer(&w, "{:g} ", var->Coeff);
            for (auto [k, v] : var->Letters) {
                fmt_to_writer(&w, "{:c}^{} ", *k, *v);
            }
        }

        auto *nodeTitle = mprint("TERM {}##{}", w.Builder, node->ImGuiID);
        if (ImGui::TreeNode(nodeTitle)) {
            ImGui::TreePop();
        }
    }
}

void determine_new_parameters(function_entry *f, hash_table<char32_t, f64> oldParams, ast *node) {
    if (!node) return;

    determine_new_parameters(f, oldParams, node->Left);
    determine_new_parameters(f, oldParams, node->Right);

    if (node->Type == ast::TERM) {
        for (auto [k, _] : ((ast_term *) node)->Letters) {
            if (*k != 'x') {  // @TODO
                if (has(oldParams, *k)) {
                    *(f->Parameters[*k]) = *oldParams[*k];
                } else {
                    *(f->Parameters[*k]) = 0.0;
                }
            }
        }
    }
}

void ui_functions() {
    ImGui::Begin("Functions", null);
    {
        s64 indexToRemove = -1;

        For_enumerate(GraphState->Functions) {
            ImGui::PushID(it.ImGuiID);

            if (ImGui::InputText("", it.Formula, function_entry::FORMULA_INPUT_BUFFER_SIZE)) {
                if (it.FormulaRoot) {
                    free_ast(it.FormulaRoot);
                    it.FormulaRoot = null;
                }

                string error = validate_and_parse_formula(&it);
                clone(&it.FormulaMessage, error);

                if (!it.FormulaMessage) {
                    hash_table<char32_t, f64> oldParams;
                    WITH_ALLOC(Context.Temp) {
                        clone(&oldParams, it.Parameters);
                    }
                    free(it.Parameters);
                    determine_new_parameters(&it, oldParams, it.FormulaRoot);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("X")) {
                indexToRemove = it_index;
            }

            if (it.FormulaMessage) {
                ImGui::Text(temp_to_c_string(it.FormulaMessage));
            } else if (GraphState->DisplayAST) {
                display_ast(it.FormulaRoot);
            }

            for (auto [k, v] : it.Parameters) {
                f64 min = -100, max = 100;
                ImGui::SliderScalar(mprint("{:c}", *k), ImGuiDataType_Double, v, &min, &max, "%.7f", ImGuiSliderFlags_NoRoundToFormat);
            }

            ImGui::PopID();
        }

        if (indexToRemove != -1) {
            free(GraphState->Functions[indexToRemove]);
            remove_at_index(GraphState->Functions, indexToRemove);
        }

        if (ImGui::Button("Add entry")) {
            append(GraphState->Functions);
        }
    }
    ImGui::End();
}