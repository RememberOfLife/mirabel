#include "imgui.h"
#include "surena/games/tictactoe_ultimate.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "games/tictactoe_ultimate.hpp"

namespace Games {

            TicTacToe_Ultimate::TicTacToe_Ultimate():
                BaseGameVariant("Ultimate")
            {}
            
            TicTacToe_Ultimate::~TicTacToe_Ultimate()
            {}

            surena::Game* TicTacToe_Ultimate::new_game()
            {
                return new surena::TicTacToe_Ultimate();
            }

            void TicTacToe_Ultimate::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void TicTacToe_Ultimate::draw_state_editor()
            {
                // TODO proper state editor
                ImGui::TextDisabled("<state editor unavailable>");
                // surena::TicTacToe_Ultimate* game = // get from state ctrl
                // for (int iy = 0; iy < 3; iy++) {
                //     for (int ix = 0; ix < 3; ix++) {
                //         uint8_t state_value = 
                //     }
                // }
            }

            const char* TicTacToe_Ultimate::description()
            {
                //TODO
                return "TicTacToe_Ultimate Description";
            }

}
