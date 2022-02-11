#include <cstdint>

#include "imgui.h"
#include "surena/games/tictactoe.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

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
                            // modified state via imgui, queue internal update event
                            StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_internal_update_event(StateControl::EVENT_TYPE_GAME_INTERNAL_UPDATE, (imgui_check<<4)|(iy<<2)|(ix)));
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
                    // queue internal update event
                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_internal_update_event(StateControl::EVENT_TYPE_GAME_INTERNAL_UPDATE, (1<<6)|(imgui_current<<4)));
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
                    // queue internal update event
                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_internal_update_event(StateControl::EVENT_TYPE_GAME_INTERNAL_UPDATE, (2<<6)|(imgui_result<<4)));
                }
            }

            const char* TicTacToe::description()
            {
                //TODO
                return "TicTacToe Description";
            }

}
