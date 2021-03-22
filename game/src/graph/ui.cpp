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
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("Info");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("This is an awesome calculator.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("* Camera controls");
            ImGui::TextUnformatted("      Ctrl + Left Mouse -> Pan");
            ImGui::TextUnformatted("      Ctrl + Right Mouse -> Scale");
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
    auto *cam = &GameState->Camera;

    ImGui::Begin("Scene", null);
    {
        ImGui::Text("Frame information:");
        ImGui::Text("  %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Clear color:");
        ImGui::ColorPicker3("", &GameState->ClearColor.x, ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Reset color")) GameState->ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};
    }
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
    ImGui::End();
}

// Returns an error message
string validate_and_parse_formula() {
    token_stream tokens;

    WITH_ALLOC(Context.Temp) {
        tokens = tokenize(string(GameState->Formula));
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

    GameState->FormulaRoot = root;

    return "";
}

void display_formula_ast(ast *node) {
    if (!node) return;

    if (node->Type == ast::OP) {
        auto *nodeTitle = to_c_string(tsprint("OP {:c}##{}", ((ast_op *) node)->Op, node->ID), Context.Temp);

        if (ImGui::TreeNode(nodeTitle)) {
            display_formula_ast(node->Left);
            display_formula_ast(node->Right);

            ImGui::TreePop();
        }
    } else if (node->Type == ast::VARIABLE) {
        auto *nodeTitle = to_c_string(tsprint("VARIABLE##{}", node->ID), Context.Temp);

        if (ImGui::TreeNode(nodeTitle)) {
            auto *var = (ast_variable *) node;
            ImGui::Text(to_c_string(tsprint("Coeff: {:g}", var->Coeff), Context.Temp));
            for (auto [k, v] : var->Letters) {
                ImGui::Text(to_c_string(tsprint("{:c}^{}", *k, *v), Context.Temp));
            }
            ImGui::TreePop();
        }
    }
}

void ui_functions() {
    ImGui::Begin("Functions", null);
    {
        ImGui::Text("Enter expression:");
        if (ImGui::InputText("", GameState->Formula, GameState->FORMULA_INPUT_BUFFER_SIZE)) {
            if (GameState->FormulaRoot) {
                free_ast(GameState->FormulaRoot);
                GameState->FormulaRoot = null;
            }

            string error = validate_and_parse_formula();
            clone(&GameState->FormulaMessage, error);
        }

        if (GameState->FormulaMessage) {
            ImGui::Text(to_c_string(GameState->FormulaMessage, Context.Temp));
        } else {
            display_formula_ast(GameState->FormulaRoot);
        }
    }
    ImGui::End();
}