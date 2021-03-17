#include "state.h"

void editor_main() {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("CDock Window", null,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                     ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("CDock");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Nastroyki")) {
            if (ImGui::MenuItem("VSync (ako znaesh kakvo pravi tova)", "", GameMemory->MainWindow->Flags & window::VSYNC))
                GameMemory->MainWindow->Flags ^= window::VSYNC;
            ImGui::EndMenu();
        }
        ImGui::TextDisabled("Info");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Towa tuk e malko informaciya.");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("* Eto kak se kontrolira kamerata:");
            ImGui::TextUnformatted("      Ctrl + Left Mouse -> zavurtane");
            ImGui::TextUnformatted("      Ctrl + Middle Mouse -> dvijenie");
            ImGui::TextUnformatted("      Ctrl + Right Mouse -> zoom");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted(" Samo deto nyamashe wreme i kamerata ne prawi nishto :#");
            ImGui::TextUnformatted("");
            ImGui::TextUnformatted("Tozi proekt juridicheski e zashiten sus swobodniya MIT licens, t.e. mozhesh da prawish kakwoto si iskash s nego.");
            ImGui::TextUnformatted("Eto kachen e tyk: github.com/Repertoi-e/light-std-graphics/");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

import fmt;

string roots_display() {
    f32 a = GameState->QuadraticA;
    f32 b = GameState->QuadraticB;
    f32 c = GameState->QuadraticC;

    if (abs(a) < 1e-7f) {  // Pochti pochti 0
        return "A e nyla i urawnenieto\nne e kwadratno :#";
    }

    f32 d = b * b - 4 * a * c;
    if (d < 0) {
        return "Korenite sa kompleksirani";
    } else if (abs(d) < 1e-7f) {  // Diskriminanata e pochti pochti 0
        f32 x = (-b / (2 * a));
        
        if (x == 0) x = 0;
        
        return sprint("Ima edin koren:\nx = {}", x);
    } else {
        f32 sd = Math_Sqrt_flt32(d);

        f32 x1 = (-b + sd) / (2 * a);
        f32 x2 = (-b - sd) / (2 * a);
        
        if (x1 == 0) x1 = 0;
        if (x2 == 0) x2 = 0;
        
        return sprint("Ima dva korena:\nx_1 = {}\nx_2 = {}", x1, x2);
    }
}

void editor_scene_properties() {
    auto *cam = &GameState->Camera;

    ImGui::Begin("Scena", null);
    {
        ImGui::InputFloat("PPM (pikseli za metur)", &GameState->PixelsPerMeter);
        ImGui::Text("Frame information:");
        ImGui::Text("  %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Clear color:");
        ImGui::ColorPicker3("", &GameState->ClearColor.x, ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Reset color")) GameState->ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};
    }
    {
        if (ImGui::Button("Reset camera")) camera_reinit(cam);

        ImGui::Text("Position: %.3f, %.3f", cam->Position.x, cam->Position.y);
        ImGui::Text("Roll: %.3f", cam->Roll);
        ImGui::Text("Scale (zoom): %.3f, %.3f", cam->Scale.x, cam->Scale.y);
        if (ImGui::Button("Reset rotation")) cam->Roll = 0;

        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Pan speed", &cam->PanSpeed);
        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Rotation speed", &cam->RotationSpeed);
        ImGui::PushItemWidth(-140);
        ImGui::InputFloat("Zoom speed", &cam->ZoomSpeed);
        ImGui::InputFloat2("Zoom min/max", &cam->ZoomMin);
        if (ImGui::Button("Default camera constants")) camera_reset_constants(cam);
    }

    ImGui::End();

    ImGui::Begin("Glaven prozorec", null);
    {
        ImGui::Checkbox("Zabavno", &GameState->InputSlider);

        if (GameState->InputSlider) {
            ImGui::SliderFloat("A", &GameState->QuadraticA, -100.0f, 100.0f);
            ImGui::SliderFloat("B", &GameState->QuadraticB, -100.0f, 100.0f);
            ImGui::SliderFloat("C", &GameState->QuadraticC, -100.0f, 100.0f);
        } else {
            ImGui::InputFloat("A", &GameState->QuadraticA);
            ImGui::InputFloat("B", &GameState->QuadraticB);
            ImGui::InputFloat("C", &GameState->QuadraticC);
        }
        ImGui::Text(to_c_string(roots_display(), Context.Temp));
    }
    ImGui::End();
}
