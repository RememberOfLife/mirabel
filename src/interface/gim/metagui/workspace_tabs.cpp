#include <cstdint>
#include <cstdio>

#include "imgui.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_workspace_tabs()
{
    ImGui::SetNextWindowPos(imgui_viewport->WorkPos);
    ImGui::SetNextWindowSize(imgui_viewport->WorkSize);
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("WorkspaceTabs", NULL, flags)) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            if (ImGui::BeginTabItem("Tab 1")) {

                {
                    ImVec2 size = ImGui::GetContentRegionAvail();

                    ImGui::BeginChild("OpenGLRenderArea1", size, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

                    ImVec2 render_pos = ImGui::GetCursorScreenPos();
                    ImVec2 render_size = ImGui::GetContentRegionAvail();
                    fedd.fex = render_pos.x;
                    fedd.fey = render_pos.y;
                    fedd.few = render_size.x;
                    fedd.feh = render_size.y;
                    metagui_empty_frontend();

                    // disable ImGui inputs in this region
                    ImGui::SetItemAllowOverlap();

                    ImGui::EndChild();
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tab 2")) {
                ImGui::Text("This is Tab 2");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tab 3")) {
                ImGui::Text("This is Tab 3");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
