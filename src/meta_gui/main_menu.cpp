#include <cstdint>

#include "imgui.h"

#include "state_control/controller.hpp"
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
                if (ImGui::MenuItem("Gamestate Config", "CTRL + G", show_gamestate_config_window)) {
                    show_gamestate_config_window = !show_gamestate_config_window;
                }
                if (ImGui::MenuItem("Guistate Config", "CTRL + F", show_guistate_config_window)) {
                    show_guistate_config_window = !show_guistate_config_window;
                }
                if (ImGui::MenuItem("Engine", "CTRL + E", show_engine_window)) {
                    show_engine_window = !show_engine_window;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "CTRL + Q", false)) {
                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_EXIT));
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("INIT TEST")) {
                MetaGui::log("#S init test\n");
                StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_FRONTEND_LOAD));
            }
            ImGui::EndMainMenuBar();
        }
    }

}
