#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    bool fullscreen = false;

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
                if (ImGui::MenuItem("Config Registry", "CTRL + K", show_config_registry_window)) {
                    show_config_registry_window = !show_config_registry_window;
                }
                if (ImGui::MenuItem("Connection", "CTRL + C", show_connection_window)) {
                    show_connection_window = !show_connection_window;
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
                if (ImGui::MenuItem("Chat", "CTRL + T", show_chat_window)) {
                    show_chat_window = !show_chat_window;
                }
                if (ImGui::MenuItem("Time Control", "CTRL + U", show_timectl_window)) {
                    show_timectl_window = !show_timectl_window;
                }
                if (ImGui::MenuItem("History", "CTRL + H", show_history_window)) {
                    show_history_window = !show_history_window;
                }
                if (ImGui::MenuItem("Plugins", "CTRL + P", show_plugins_window)) {
                    show_plugins_window = !show_plugins_window;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Fullscreen", "F11", fullscreen)) {
                    fullscreen = !fullscreen;
                    // borderless fullscreen
                    SDL_SetWindowFullscreen(Control::main_client->sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "CTRL + Q", false)) {
                    event_any es;
                    event_create_type(&es, EVENT_TYPE_EXIT);
                    event_queue_push(&Control::main_client->inbox, &es);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About", NULL, show_about_window)) {
                    show_about_window = !show_about_window;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

} // namespace MetaGui
