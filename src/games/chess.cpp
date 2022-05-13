#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/chess.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "games/chess.hpp"

namespace Games {

            Chess::Chess():
                BaseGameVariant("Standard")
            {}
            
            Chess::~Chess()
            {}

            game* Chess::new_game()
            {
                game* new_game = (game*)malloc(sizeof(game));
                *new_game = game{
                    .sync_ctr = 0,
                    .data = NULL,
                    .options = NULL,
                    .methods = &chess_gbe,
                };
                new_game->methods->create(new_game);
                new_game->methods->import_state(new_game, NULL);
                return new_game;
            }

            void Chess::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void Chess::draw_state_editor(game* abstract_game)
            {
                if (abstract_game == NULL) {
                    return;
                }
                //TODO proper state editor
                const char* check_options[4] = {"-", "WHITE", "BLACK"};
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
                //TODO expose winningcondition
            }

            const char* Chess::description()
            {
                //TODO
                return "Chess Description";
            }

}
