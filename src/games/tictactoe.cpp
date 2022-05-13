#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/tictactoe.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "games/tictactoe.hpp"

namespace Games {

            TicTacToe::TicTacToe():
                BaseGameVariant("Standard")
            {}
            
            TicTacToe::~TicTacToe()
            {}

            game* TicTacToe::new_game()
            {
                game* new_game = (game*)malloc(sizeof(game));
                *new_game = game{
                    .sync_ctr = 0,
                    .data = NULL,
                    .options = NULL,
                    .methods = &tictactoe_gbe,
                };
                new_game->methods->create(new_game);
                new_game->methods->import_state(new_game, NULL);
                return new_game;
            }

            void TicTacToe::draw_options()
            {
                ImGui::TextDisabled("<no options>");
            }

            void TicTacToe::draw_state_editor(game* abstract_game)
            {
                if (abstract_game == NULL) {
                    return;
                }
                tictactoe_internal_methods* ag_int = (tictactoe_internal_methods*)abstract_game->methods->internal_methods;
                const char* check_options[3] = {"-", "X", "O"};
                float check_width = 1.2*ImGui::CalcTextSize("XO").x;
                int imgui_id = 0;
                // edit: board state
                ImGui::Text("board state:");
                for (int iy = 2; iy >= 0; iy--) {
                    for (int ix = 0; ix < 3; ix++) {
                        if (ix > 0) {
                            ImGui::SameLine();
                        }
                        player_id board_state;
                        ag_int->get_cell(abstract_game, ix, iy, &board_state);
                        player_id imgui_check = board_state;
                        ImGui::PushID(imgui_id++);
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
                            // modified state via imgui, clone+edit+load
                            game* game_clone = (game*)malloc(sizeof(game));
                            abstract_game->methods->clone(abstract_game, game_clone);
                            ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_cell(game_clone, ix, iy, imgui_check);
                            size_t game_state_buffer_len;
                            game_clone->methods->export_state(game_clone, &game_state_buffer_len, NULL);
                            char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                            game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
                            Control::main_client->inbox.push(
                                Control::event(Control::EVENT_TYPE_GAME_IMPORT_STATE, game_state_buffer_len, game_state_buffer));
                            game_clone->methods->destroy(game_clone);
                            free(game_clone);
                        }
                    }
                }
                // edit: player to move
                player_id pbuf;
                uint8_t pbuf_c;
                abstract_game->methods->players_to_move(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = PLAYER_NONE;
                }
                player_id board_current = pbuf;
                player_id imgui_current = board_current;
                ImGui::PushID(imgui_id++);
                ImGui::PushItemWidth(check_width);
                if (ImGui::BeginCombo("player to move", check_options[imgui_current], ImGuiComboFlags_NoArrowButton)) {
                    for (int i = 0; i < 3; i++) {
                        bool is_selected = (imgui_current == i);
                        if (ImGui::Selectable(check_options[i], is_selected)) {
                            imgui_current = i;
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
                if (imgui_current != board_current) {
                    // modified state via imgui, clone+edit+load
                    game* game_clone = (game*)malloc(sizeof(game));
                    abstract_game->methods->clone(abstract_game, game_clone);
                    ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_current_player(game_clone, imgui_current);
                    size_t game_state_buffer_len;
                    game_clone->methods->export_state(game_clone, &game_state_buffer_len, NULL);
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
                    Control::main_client->inbox.push(
                        Control::event(Control::EVENT_TYPE_GAME_IMPORT_STATE, game_state_buffer_len, game_state_buffer));
                    game_clone->methods->destroy(game_clone);
                    free(game_clone);
                }
                // edit: result
                abstract_game->methods->get_results(abstract_game, &pbuf_c, &pbuf);
                if (pbuf_c == 0) {
                    pbuf = PLAYER_NONE;
                }
                player_id board_result = pbuf;
                player_id imgui_result = board_result;
                ImGui::PushID(imgui_id++);
                ImGui::PushItemWidth(check_width);
                if (ImGui::BeginCombo("result", check_options[imgui_result], ImGuiComboFlags_NoArrowButton)) {
                    for (int i = 0; i < 3; i++) {
                        bool is_selected = (imgui_result == i);
                        if (ImGui::Selectable(check_options[i], is_selected)) {
                            imgui_result = i;
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
                if (imgui_result != board_result) {
                    // modified state via imgui, clone+edit+load
                    game* game_clone = (game*)malloc(sizeof(game));
                    abstract_game->methods->clone(abstract_game, game_clone);
                    ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_result(game_clone, imgui_result);
                    size_t game_state_buffer_len;
                    game_clone->methods->export_state(game_clone, &game_state_buffer_len, NULL);
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
                    Control::main_client->inbox.push(
                        Control::event(Control::EVENT_TYPE_GAME_IMPORT_STATE, game_state_buffer_len, game_state_buffer));
                    game_clone->methods->destroy(game_clone);
                    free(game_clone);
                }
            }

            const char* TicTacToe::description()
            {
                //TODO
                return "TicTacToe Description";
            }

}
