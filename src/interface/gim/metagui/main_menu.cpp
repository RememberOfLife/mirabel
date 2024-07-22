#include <cstdint>
#include <cstdio>

#include "imgui.h"

#include "mirabel/app_interface.h"
#include "mirabel/application.h"
#include "mirabel/event.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_main_menu_bar()
{
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("Windows")) {
            ImGui::BeginDisabled();
            if (ImGui::MenuItem("Stats", "F3", false)) {
                //TODO
            }
            if (ImGui::MenuItem("Logs", "F4", false)) {
                //TODO
            }
            ImGui::Separator();
            ImGui::EndDisabled();
            if (ImGui::MenuItem("ImGui Demo Mode", "F5", show_imgui_demo)) {
                show_imgui_demo = !show_imgui_demo;
            }
            ImGui::BeginDisabled();
            if (ImGui::MenuItem("Fullscreen", "F11", false)) {
                //TODO
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "CTRL + Q", false)) {
                event_any es;
                event_create_type(&es, EVENT_TYPE_EXIT);
                event_queue_push(&appi.interface->inbox, &es);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About", NULL, show_about_info)) {
                show_about_info = !show_about_info;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
