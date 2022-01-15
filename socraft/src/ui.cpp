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
            bool vsync = Memory->MainWindow.IsVsync();
            if (ImGui::MenuItem("VSync", "", vsync)) {
                Memory->MainWindow.SetVsync(!vsync);
            }
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("This is a minecraft clone written (expect ImGui) from scratch in order to battle-test my C++ standard library replacement.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted(" - Click the viewport to move around, press ESC to restore the mouse.");
            ImGui::TextUnformatted(" - Press F1 to toggle editor UI.");
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
        ImGui::SliderFloat("Sensitivity", &Game->Camera.Sensitivity, 0.0f, 0.03f);
        ImGui::Text("");
        ImGui::SliderFloat3("Pos", &Game->Camera.Position.x, -4, 4);
        ImGui::SliderFloat3("Rot", &Game->Camera.Rotation.x, -4, 4);
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

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            Memory->MainWindow.GrabMouse();
            Game->MouseGrabbed = true;
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();
}
