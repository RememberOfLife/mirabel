#include "imgui.h"
#include "surena_game.hpp"
#include "surena_tictactoe.hpp"

#include "games/game_catalogue.hpp"

#include "games/tictactoe.hpp"

namespace Games {

            TicTacToe::TicTacToe():
                BaseGameVariant("Standard")
            {
                //TODO
            }
            
            TicTacToe::~TicTacToe()
            {
                //TODO
            }

            surena::PerfectInformationGame* TicTacToe::new_game()
            {
                return new surena::TicTacToe();
            }

            void TicTacToe::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void TicTacToe::draw_state_editor()
            {
                // TODO proper state editor
                ImGui::TextDisabled("<state editor unavailable>");
                // surena::TicTacToe* game = // get from state ctrl
                // for (int iy = 0; iy < 3; iy++) {
                //     for (int ix = 0; ix < 3; ix++) {
                //         uint8_t state_value = 
                //     }
                // }
            }

            const char* TicTacToe::description()
            {
                //TODO
                return "TicTacToe Description";
            }

}
