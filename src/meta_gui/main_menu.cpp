#include <cstdint>
#include <cstdio>

#include "imgui.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"

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
                { //TODO REENABLE engine
                    // if (ImGui::MenuItem("Engine", "CTRL + E", show_engine_window)) {
                    // show_engine_window = !show_engine_window;
                    // }
                    ImGui::MenuItem("Engine", "CTRL + E", false, false);
                }
                if (ImGui::MenuItem("Chat", "CTRL + T", show_chat_window)) {
                    show_chat_window = !show_chat_window;
                }
                if (ImGui::MenuItem("Time Control", "CTRL + U", show_timectl_window)) {
                    show_timectl_window = !show_timectl_window;
                }
                { //TODO REENABLE history
                    // if (ImGui::MenuItem("History", "CTRL + H", show_history_window)) {
                    //     show_history_window = !show_history_window;
                    // }
                    ImGui::MenuItem("History", "CTRL + H", false, false);
                }
                if (ImGui::MenuItem("Lobby", "CTRL + L", show_lobby_window)) {
                    show_lobby_window = !show_lobby_window;
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

            char pov_str[12];
            if (Control::main_client->dd.view == PLAYER_NONE) {
                sprintf(pov_str, "POV NONE");
            } else if (Control::main_client->dd.view == PLAYER_RAND) {
                sprintf(pov_str, "POV RAND");
            } else {
                sprintf(pov_str, "POV %03hhu", Control::main_client->dd.view);
            }
            ImVec2 size = ImGui::CalcTextSize(pov_str);
            ImGui::SameLine();
            ImGuiStyle& style = ImGui::GetStyle();
            size.x += style.FramePadding.x * 2 + style.ItemSpacing.x;
            ImGui::SetCursorPos(ImVec2(ImGui::GetIO().DisplaySize.x - size.x, 0));
            bool enable_switch = Control::main_client->the_game == NULL;
            if (enable_switch) {
                ImGui::BeginDisabled();
            }
            if (ImGui::BeginMenu(pov_str) && !enable_switch) {
                uint8_t pc;
                game_player_count(Control::main_client->the_game, &pc);
                for (uint8_t i = 0; i < pc + 1 + (game_ff(Control::main_client->the_game).random_moves ? 1 : 0); i++) {
                    //TODO the selection here should probably be an event and not directly set
                    //TODO also the available selectables, and if they are actually available, should be controlled by the lobby, otherwise disabled
                    if (i == 0) {
                        if (ImGui::MenuItem("NONE")) {
                            Control::main_client->dd.view = PLAYER_NONE;
                        }
                    } else if (i == pc + 1 && game_ff(Control::main_client->the_game).random_moves) {
                        if (ImGui::MenuItem("RAND")) {
                            Control::main_client->dd.view = PLAYER_RAND;
                        }
                    } else {
                        sprintf(pov_str, "%03hhu", i);
                        if (ImGui::MenuItem(pov_str)) {
                            Control::main_client->dd.view = i;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (enable_switch) {
                ImGui::EndDisabled();
            }

            ImGui::EndMainMenuBar();
        }
    }

} // namespace MetaGui
