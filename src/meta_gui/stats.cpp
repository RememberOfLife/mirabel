#include <cstdint>

#include "SDL.h"
#include "imgui.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void stats_overlay(bool* p_open)
    {
        static int corner = 0;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (corner != -1)
        {
            const float PAD = 10.0f;
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos = viewport->WorkPos; // use work area to avoid menu-bar/task-bar, if any!
            ImVec2 work_size = viewport->WorkSize;
            ImVec2 window_pos, window_pos_pivot;
            window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
            window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
            window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
            window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            window_flags |= ImGuiWindowFlags_NoMove;
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // transparent background
        if (ImGui::Begin("Stats Overlay", p_open, window_flags))
        {
            // ImGui::Text("Stats Overlay");
            // ImGui::Separator();
            static uint64_t last_ticks = 1;
            uint64_t current_ticks = SDL_GetTicks64();
            static float running_fps = 0;
            static uint64_t last_fps_update_ticks = 0;
            static float last_fps = 0;
            if (current_ticks != last_ticks) {
                float current_fps = 1000 / (current_ticks - last_ticks);
                running_fps = 0.5 * running_fps + 0.5 * current_fps;
                if (current_ticks - last_fps_update_ticks > 256) {
                    last_fps_update_ticks = current_ticks;
                    last_fps = running_fps;
                }
                last_ticks = current_ticks;
            }
            ImGui::Text("FPS: %.1f", last_fps);
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (p_open && ImGui::MenuItem("Close")) *p_open = false;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

}
