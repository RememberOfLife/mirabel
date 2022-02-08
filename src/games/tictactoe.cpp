#include <cstdint>

#include "imgui.h"
#include "surena/games/tictactoe.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

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

            void TicTacToe::draw_state_editor(surena::Game* game)
            {
                surena::TicTacToe* state = dynamic_cast<surena::TicTacToe*>(game);
                if (state == nullptr) {
                    return;
                }
                //TODO updating the state this way is unsafe as it does not propagate the update like a move does, see readme
                ImGui::TextColored(ImVec4{0.87, 0.68, 0.17, 1}, "WARNING: updates unsafe");
                const char* check_options[3] = {".", "X", "O"};
                float check_width = 1.2*ImGui::CalcTextSize("XO").x;
                ImGui::Text("board state:");
                for (int iy = 2; iy >= 0; iy--) {
                    for (int ix = 0; ix < 3; ix++) {
                        if (ix > 0) {
                            ImGui::SameLine();
                        }
                        int board_state = state->get_cell(ix, iy);
                        int imgui_check = board_state;
                        ImGui::PushID(iy*3+ix);
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
                            // modified state via imgui, update real thing
                            state->set_cell(ix, iy, imgui_check);
                            MetaGui::log("#W unsafe state update in tictactoe\n"); //TODO unsafe state update
                        }
                    }
                }
                //TODO edit result and current player
            }

            const char* TicTacToe::description()
            {
                //TODO
                return "TicTacToe Description";
            }

}
