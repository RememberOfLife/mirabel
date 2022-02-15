#include <cstdint>

#include "imgui.h"
#include "surena/games/chess.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "games/chess.hpp"

namespace Games {

            Chess::Chess():
                BaseGameVariant("Standard")
            {}
            
            Chess::~Chess()
            {}

            surena::Game* Chess::new_game()
            {
                return new surena::Chess();
            }

            void Chess::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void Chess::draw_state_editor(surena::Game* abstract_game)
            {
                surena::Chess* game = dynamic_cast<surena::Chess*>(abstract_game);
                if (game == nullptr) {
                    return;
                }
                //TODO proper state editor
                const char* check_options[4] = {"-", "WHITE", "BLACK"};
                ImGui::Text("player to move: %s", check_options[game->player_to_move()]);
                ImGui::Text("result: %s", check_options[game->get_result()]);
                //TODO expose winningcondition
            }

            const char* Chess::description()
            {
                //TODO
                return "Chess Description";
            }

}
