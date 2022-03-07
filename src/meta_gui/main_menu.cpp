#include <cstdint>

#include "imgui.h"

#include "state_control/client.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void main_menu_bar(bool* p_open)
    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                if (ImGui::MenuItem("Stats", "F3", show_stats_overlay)) {
                    show_stats_overlay = !show_stats_overlay;
                }
                if (ImGui::MenuItem("Logs", "F4", show_logs_window)) {
                    show_logs_window = !show_logs_window;
                }
                if (ImGui::MenuItem("Game Config", "CTRL + G", show_game_config_window)) {
                    show_game_config_window = !show_game_config_window;
                }
                if (ImGui::MenuItem("Frontend Config", "CTRL + F", show_frontend_config_window)) {
                    show_frontend_config_window = !show_frontend_config_window;
                }
                if (ImGui::MenuItem("Engine", "CTRL + E", show_engine_window)) {
                    show_engine_window = !show_engine_window;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "CTRL + Q", false)) {
                    StateControl::main_client->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_EXIT));
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

}
