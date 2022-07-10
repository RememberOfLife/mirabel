#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/client.hpp"
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

                // temporary state str display
                static char* state_str = NULL;
                static uint64_t state_step = 0;
                static bool changed = false;
                if (state_step != Control::main_client->game_step) {
                    free(state_str);
                    state_str = (char*)malloc(abstract_game->sizer.state_str);
                    size_t _len;
                    abstract_game->methods->export_state(abstract_game, &_len, state_str);
                    state_step = Control::main_client->game_step;
                    changed = false;
                }
                if (state_str) {
                    if (ImGui::InputText("state", state_str, abstract_game->sizer.state_str)) {
                        changed |= true;
                    }
                    if (changed) {
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
                        if (ImGui::Button("S")) {
                            changed = false;
                            f_event_any es;
                            f_event_create_game_state(&es, F_EVENT_CLIENT_NONE, state_str);
                            f_event_queue_push(&Control::main_client->inbox, &es);
                        }
                        ImGui::PopStyleColor(3);
                    }
                }
                if (ImGui::Button("reload")) {
                    f_event_any es;
                    f_event_create_game_state(&es, F_EVENT_CLIENT_NONE, state_str);
                    f_event_queue_push(&Control::main_client->inbox, &es);
                }

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
