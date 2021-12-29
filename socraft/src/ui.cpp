#include "game.h"

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
            if (ImGui::MenuItem("UI (F1 to toggle)", "", Game->UI)) {
                Game->UI = !Game->UI;
            }

            bool vsync = Memory->MainWindow.get_flags() & window::VSYNC;
            if (ImGui::MenuItem("VSync", "", vsync)) {
                Memory->MainWindow.set_vsync(!vsync);
            }
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("This is a minecraft clone written (expect ImGui) from scratch in order to battle-test my C++ standard library replacement.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("The project is under the MIT license.");
            ImGui::TextUnformatted("Source code: github.com/Repertoi-e/light-std-graphics/");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void ui_scene_properties() {
    ImGui::Begin("Scene", null);
    {
        ImGui::Text("Frame information:");
        ImGui::Text("  %.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("  (%.1f FPS)", ImGui::GetIO().Framerate);
        ImGui::Text("");
        ImGui::SliderFloat3("Pos", &Game->Camera.Position.x, -100, 100);
        ImGui::SliderFloat3("Rot", &Game->Camera.Rotation.x, -100, 100);
        ImGui::Text("");
        ImGui::Text("Clear color:");
        ImGui::ColorPicker3("", &Game->ClearColor.x, ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Reset color")) Game->ClearColor = {0.65f, 0.87f, 0.95f, 1.0f};
    }
    ImGui::End();
}

void ui_viewport() {
    ImGui::Begin("Viewport", null);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImVec2 vMin = ImGui::GetWindowContentRegionMin();
        ImVec2 vMax = ImGui::GetWindowContentRegionMax();

        int2 size = int2((s32) (vMax.x - vMin.x), (s32) (vMax.y - vMin.y));

        if (Game->ViewportSize != size) {
            Game->ViewportSize  = size;
            Game->ViewportDirty = true;
        }

        ImGui::Image(Game->Viewport, float2(size));
        ImGui::PopStyleVar();
    }
    ImGui::End();
}
