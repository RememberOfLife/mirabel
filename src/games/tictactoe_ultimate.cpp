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

            void TicTacToe_Ultimate::draw_state_editor(surena::Game* abstract_game)
            {
                surena::TicTacToe_Ultimate* game = dynamic_cast<surena::TicTacToe_Ultimate*>(abstract_game);
                if (game == nullptr) {
                    return;
                }
                //TODO proper state editor
                const char* check_options[3] = {"-", "X", "O"};
                ImGui::Text("player to move: %s", check_options[game->player_to_move()]);
                ImGui::Text("result: %s", check_options[game->get_result()]);
            }

            const char* TicTacToe_Ultimate::description()
            {
                //TODO
                return "TicTacToe_Ultimate Description";
            }

}
