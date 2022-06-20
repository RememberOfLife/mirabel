#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/havannah.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "games/havannah.hpp"

namespace Games {

            Havannah::Havannah():
                BaseGameVariant("Standard")
            {}
            
            Havannah::~Havannah()
            {}

            game* Havannah::new_game(const char* options)
            {
                game* new_game = (game*)malloc(sizeof(game));
                *new_game = game{
                    .methods = &havannah_gbe,
                    .data1 = NULL,
                    .data2 = NULL,
                };
                if (options) {
                    new_game->methods->create_with_opts_str(new_game, options);
                } else {
                    new_game->methods->create_with_opts_bin(new_game, &opts);
                }
                new_game->methods->import_state(new_game, NULL);
                return new_game;
            }

            void Havannah::draw_options()
            {
                ImGui::InputScalar("size", ImGuiDataType_U32, &opts.size);
                if (opts.size < 4) {
                    opts.size = 4;
                }
                if (opts.size > 10) {
                    opts.size = 10;
                }
            }

            void Havannah::draw_state_editor(game* abstract_game)
            {
                if (abstract_game == NULL) {
                    return;
                }
                //TODO proper state editor
                // white is actually displayed red per default, but because that might be configurable it is kept uniform here
                const char* move_options[4] = {"-", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
                const char* result_options[4] = {"DRAW", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
                player_id pbuf;
                uint8_t pbuf_c;
                abstract_game->methods->players_to_move(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = HAVANNAH_PLAYER_NONE;
                }
                ImGui::Text("player to move: %s", move_options[pbuf]);
                abstract_game->methods->get_results(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = HAVANNAH_PLAYER_INVALID;
                }
                ImGui::Text("result: %s", result_options[pbuf]);
                //TODO expose winningcondition
            }

            const char* Havannah::description()
            {
                //TODO
                return "Havannah Description";
            }

}
