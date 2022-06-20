#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "games/tictactoe_ultimate.hpp"

namespace Games {

            TicTacToe_Ultimate::TicTacToe_Ultimate():
                BaseGameVariant("Ultimate")
            {}
            
            TicTacToe_Ultimate::~TicTacToe_Ultimate()
            {}

            game* TicTacToe_Ultimate::new_game(const char* options)
            {
                game* new_game = (game*)malloc(sizeof(game));
                *new_game = game{
                    .methods = &tictactoe_ultimate_gbe,
                    .data1 = NULL,
                    .data2 = NULL,
                };
                new_game->methods->create_default(new_game);
                new_game->methods->import_state(new_game, NULL);
                return new_game;
            }

            void TicTacToe_Ultimate::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void TicTacToe_Ultimate::draw_state_editor(game* abstract_game)
            {
                if (abstract_game == NULL) {
                    return;
                }
                //TODO proper state editor
                const char* check_options[3] = {"-", "X", "O"};
                player_id pbuf;
                uint8_t pbuf_c;
                abstract_game->methods->players_to_move(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = PLAYER_NONE;
                }
                ImGui::Text("player to move: %s", check_options[pbuf]);
                abstract_game->methods->get_results(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = PLAYER_NONE;
                }
                ImGui::Text("result: %s", check_options[pbuf]);
            }

            const char* TicTacToe_Ultimate::description()
            {
                //TODO
                return "TicTacToe_Ultimate Description";
            }

}
