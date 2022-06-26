#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "games/twixt_pp.hpp"

namespace Games {

            TwixT_PP::TwixT_PP():
                BaseGameVariant("PP")
            {}
            
            TwixT_PP::~TwixT_PP()
            {}

            game* TwixT_PP::new_game(const char* options)
            {
                game* new_game = (game*)malloc(sizeof(game));
                *new_game = game{
                    .methods = &twixt_pp_gbe,
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

            void TwixT_PP::draw_options()
            {
                ImGui::TextDisabled("typical sizes: 24, 30, 48");
                const uint8_t min = 10;
                const uint8_t max = 48;
                ImGui::Checkbox("square", &square);
                if (square) {
                    ImGui::SliderScalar("size", ImGuiDataType_U8, &opts.wx, &min, &max, "%u", ImGuiSliderFlags_AlwaysClamp);
                    opts.wy = opts.wx;
                } else {
                    ImGui::SliderScalar("width", ImGuiDataType_U8, &opts.wx, &min, &max, "%u", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::SliderScalar("height", ImGuiDataType_U8, &opts.wy, &min, &max, "%u", ImGuiSliderFlags_AlwaysClamp);
                }
                ImGui::Checkbox("pie swap", &opts.pie_swap);
            }

            void TwixT_PP::draw_state_editor(game* abstract_game)
            {
                if (abstract_game == NULL) {
                    return;
                }
                //TODO proper state editor
                const char* move_options[4] = {"-", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
                const char* result_options[4] = {"DRAW", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
                player_id pbuf;
                uint8_t pbuf_c;
                abstract_game->methods->players_to_move(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = TWIXT_PP_PLAYER_NONE;
                }
                ImGui::Text("player to move: %s", move_options[pbuf]);
                abstract_game->methods->get_results(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = TWIXT_PP_PLAYER_INVALID;
                }
                ImGui::Text("result: %s", result_options[pbuf]);
            }

            const char* TwixT_PP::description()
            {
                //TODO
                return "TwixT.PP Description";
            }

}
