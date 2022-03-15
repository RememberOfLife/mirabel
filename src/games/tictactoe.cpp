#include <cstdint>

#include "imgui.h"
#include "surena/games/tictactoe.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"
#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "control/guithread.hpp"

#include "games/tictactoe.hpp"

namespace Games {

            TicTacToe::TicTacToe():
                BaseGameVariant("Standard")
            {}
            
            TicTacToe::~TicTacToe()
            {}

            surena::Game* TicTacToe::new_game()
            {
                return new surena::TicTacToe();
            }

            void TicTacToe::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void TicTacToe::draw_state_editor(surena::Game* abstract_game)
            {
                surena::TicTacToe* game = dynamic_cast<surena::TicTacToe*>(abstract_game);
                if (game == nullptr) {
                    return;
                }
                const char* check_options[3] = {"-", "X", "O"};
                float check_width = 1.2*ImGui::CalcTextSize("XO").x;
                int imgui_id = 0;
                // edit: board state
                ImGui::Text("board state:");
                for (int iy = 2; iy >= 0; iy--) {
                    for (int ix = 0; ix < 3; ix++) {
                        if (ix > 0) {
                            ImGui::SameLine();
                        }
                        int board_state = game->get_cell(ix, iy);
                        int imgui_check = board_state;
                        ImGui::PushID(imgui_id++);
                        ImGui::PushItemWidth(check_width);
                        if (ImGui::BeginCombo("", check_options[imgui_check], ImGuiComboFlags_NoArrowButton)) {
                            for (int i = 0; i < 3; i++) {
                                bool is_selected = (imgui_check == i);
                                if (ImGui::Selectable(check_options[i], is_selected)) {
                                    imgui_check = i;
                                }
                                // set the initial focus when opening the combo
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();
                        ImGui::PopID();
                        if (imgui_check != board_state) {
                            // modified state via imgui, clone+edit+load
                            surena::TicTacToe* game_clone = dynamic_cast<surena::TicTacToe*>(game->clone());
                            game_clone->set_cell(ix, iy, imgui_check);
                            Control::main_client->t_gui.inbox.push(Control::event::create_game_event(Control::EVENT_TYPE_GAME_LOAD, game_clone));
                        }
                    }
                }
                // edit: player to move
                int board_current = game->player_to_move();
                int imgui_current = board_current;
                ImGui::PushID(imgui_id++);
                ImGui::PushItemWidth(check_width);
                if (ImGui::BeginCombo("player to move", check_options[imgui_current], ImGuiComboFlags_NoArrowButton)) {
                    for (int i = 0; i < 3; i++) {
                        bool is_selected = (imgui_current == i);
                        if (ImGui::Selectable(check_options[i], is_selected)) {
                            imgui_current = i;
                        }
                        // set the initial focus when opening the combo
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                ImGui::PopID();
                if (imgui_current != board_current) {
                    // modified state via imgui, clone+edit+load
                    surena::TicTacToe* game_clone = dynamic_cast<surena::TicTacToe*>(game->clone());
                    game_clone->set_current_player(imgui_current);
                    Control::main_client->t_gui.inbox.push(Control::event::create_game_event(Control::EVENT_TYPE_GAME_LOAD, game_clone));
                }
                // edit: result
                int board_result = game->get_result();
                int imgui_result = board_result;
                ImGui::PushID(imgui_id++);
                ImGui::PushItemWidth(check_width);
                if (ImGui::BeginCombo("result", check_options[imgui_result], ImGuiComboFlags_NoArrowButton)) {
                    for (int i = 0; i < 3; i++) {
                        bool is_selected = (imgui_result == i);
                        if (ImGui::Selectable(check_options[i], is_selected)) {
                            imgui_result = i;
                        }
                        // set the initial focus when opening the combo
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                ImGui::PopID();
                if (imgui_result != board_result) {
                    // modified state via imgui, clone+edit+load
                    surena::TicTacToe* game_clone = dynamic_cast<surena::TicTacToe*>(game->clone());
                    game_clone->set_result(imgui_result);
                    Control::main_client->t_gui.inbox.push(Control::event::create_game_event(Control::EVENT_TYPE_GAME_LOAD, game_clone));
                }
            }

            const char* TicTacToe::description()
            {
                //TODO
                return "TicTacToe Description";
            }

}
