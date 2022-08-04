#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "games/game_catalogue.hpp"

#include "frontends/tictactoe_ultimate.hpp"

namespace Frontends {

    void TicTacToe_Ultimate::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    }

    TicTacToe_Ultimate::TicTacToe_Ultimate():
        the_game(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        board_buttons[8-(gy*3+ly)][gx*3+lx] = sbtn{
                            static_cast<float>(gx)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(lx)*(button_size+local_padding),
                            static_cast<float>(gy)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(ly)*(button_size+local_padding),
                            button_size, button_size, false, false
                        };
                    }
                }
            }
        }
    }

    TicTacToe_Ultimate::~TicTacToe_Ultimate()
    {}

    void TicTacToe_Ultimate::set_game(game* new_game)
    {
        the_game = new_game;
        the_game_int = the_game ? (tictactoe_ultimate_internal_methods*)the_game->methods->internal_methods : NULL;
    }

    void TicTacToe_Ultimate::process_event(SDL_Event event)
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                mx = event.motion.x - x_px;
                my = event.motion.y - y_px;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - x_px;
                    int mY = event.button.y - y_px;
                    mX -= w_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
                    mY -= h_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
                    uint8_t global_target;
                    the_game_int->get_global_target(the_game, &global_target);
                    for (int gy = 0; gy < 3; gy++) {
                        for (int gx = 0; gx < 3; gx++) {
                            if (global_target != (((2-gy)<<2)|gx) && global_target != ((3<<2)|3)) {
                                continue;
                            }
                            for (int ly = 0; ly < 3; ly++) {
                                for (int lx = 0; lx < 3; lx++) {
                                    int ix = gx*3+lx;
                                    int iy = 8-(gy*3+ly);
                                    board_buttons[iy][ix].update(mX, mY);
                                    if (event.type == SDL_MOUSEBUTTONUP) {
                                        player_id cell_local;
                                        the_game_int->get_cell_local(the_game, ix, iy, &cell_local);
                                        if (board_buttons[iy][ix].hovered && board_buttons[iy][ix].mousedown && cell_local == 0) {
                                            uint64_t move_code = ix | (iy<<4);
                                            f_event_any es;
                                            f_event_create_game_move(&es, move_code);
                                            f_event_queue_push(&Control::main_client->inbox, &es);
                                        }
                                        board_buttons[iy][ix].mousedown = false;
                                    }
                                    board_buttons[iy][ix].mousedown |= (board_buttons[iy][ix].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                                }
                            }
                        }
                    }
                }
            } break;
        }
    }

    void TicTacToe_Ultimate::update()
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
        mY -= h_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
        uint8_t global_target;
        the_game_int->get_global_target(the_game, &global_target);
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                if (global_target != (((2-gy)<<2)|gx) && global_target != ((3<<2)|3)) {
                    continue;
                }
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        int ix = gx*3+lx;
                        int iy = 8-(gy*3+ly);
                        board_buttons[8-(gy*3+ly)][gx*3+lx].x = static_cast<float>(gx)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(lx)*(button_size+local_padding);
                        board_buttons[8-(gy*3+ly)][gx*3+lx].y = static_cast<float>(gy)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(ly)*(button_size+local_padding);
                        board_buttons[8-(gy*3+ly)][gx*3+lx].w = button_size;
                        board_buttons[8-(gy*3+ly)][gx*3+lx].h = button_size;
                        board_buttons[iy][ix].update(mX, mY);
                    }
                }
            }
        }
    }

    void TicTacToe_Ultimate::render()
    {
        float local_baord_size = 3*button_size+2*local_padding;
        uint8_t global_target = 0;
        if (the_game) {
            the_game_int->get_global_target(the_game, &global_target);
        }
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2-(3*local_baord_size+2*global_padding)/2, h_px/2-(3*local_baord_size+2*global_padding)/2);
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                uint8_t local_result = 0;
                if (the_game) {
                    the_game_int->get_cell_global(the_game, gx, 2-gy, &local_result);
                }
                float base_x = gx*(local_baord_size+global_padding);
                float base_y = gy*(local_baord_size+global_padding);
                if (local_result > 0) {
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, local_baord_size, local_baord_size);
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                    nvgFill(dc);
                    nvgBeginPath(dc);
                    nvgStrokeWidth(dc, local_baord_size*0.175);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    switch (local_result) {
                        case 1: {
                            // X
                            nvgMoveTo(dc, base_x+local_baord_size*0.175, base_y+local_baord_size*0.175);
                            nvgLineTo(dc, base_x+local_baord_size*0.825, base_y+local_baord_size*0.825);
                            nvgMoveTo(dc, base_x+local_baord_size*0.175, base_y+local_baord_size*0.825);
                            nvgLineTo(dc, base_x+local_baord_size*0.825, base_y+local_baord_size*0.175);
                            nvgStroke(dc);
                        } break;
                        case 2: {
                            // O
                            nvgCircle(dc, base_x+local_baord_size/2, base_y+local_baord_size/2, local_baord_size*0.3);
                            nvgStroke(dc);
                        } break;
                        case 3: {
                            //TODO draw something for a draw
                        } break;
                    }
                    continue;
                }
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        float base_x = static_cast<float>(gx)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(lx)*(button_size+local_padding);
                        float base_y = static_cast<float>(gy)*(3*button_size+2*local_padding+global_padding)+static_cast<float>(ly)*(button_size+local_padding);
                        nvgStrokeWidth(dc, button_size*0.175);
                        nvgBeginPath(dc);
                        nvgRect(dc, base_x, base_y, button_size, button_size);
                        if (the_game && pbuf_c != 0 && (global_target == (((2-gy)<<2)|gx) || global_target == ((3<<2)|3))) {
                            nvgFillColor(dc, nvgRGB(240, 217, 181));
                        } else {
                            nvgFillColor(dc, nvgRGB(161, 119, 67));
                        }
                        nvgFill(dc);
                        if (!the_game) {
                            continue;
                        }
                        int ix = gx*3+lx;
                        int iy = 8-(gy*3+ly);
                        uint8_t player_in_cell;
                        the_game_int->get_cell_local(the_game, ix, iy, &player_in_cell);
                        if (player_in_cell == 1) {
                            // X
                            nvgBeginPath(dc);
                            nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                            nvgMoveTo(dc, base_x+button_size*0.175, base_y+button_size*0.175);
                            nvgLineTo(dc, base_x+button_size*0.825, base_y+button_size*0.825);
                            nvgMoveTo(dc, base_x+button_size*0.175, base_y+button_size*0.825);
                            nvgLineTo(dc, base_x+button_size*0.825, base_y+button_size*0.175);
                            nvgStroke(dc);
                        } else if (player_in_cell == 2) {
                            // O
                            nvgBeginPath(dc);
                            nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                            nvgCircle(dc, base_x+button_size/2, base_y+button_size/2, button_size*0.3);
                            nvgStroke(dc);
                        } else if (board_buttons[iy][ix].hovered && pbuf_c > 0) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(220, 197, 161));
                            nvgRect(dc, board_buttons[iy][ix].x+button_size*0.05, board_buttons[iy][ix].y+button_size*0.05, board_buttons[iy][ix].w-button_size*0.1, board_buttons[iy][ix].h-button_size*0.1);
                            nvgFill(dc);
                        }
                        //TODO
                        /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((iy<<4)|ix)) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(125, 187, 248));
                            nvgCircle(dc, base_x+button_size/2, base_y+button_size/2, button_size*0.15);
                            nvgFill(dc);
                        }*/
                    }
                }
            }
        }
        nvgRestore(dc);
    }

    void TicTacToe_Ultimate::draw_options()
    {
        ImGui::SliderFloat("button size", &button_size, 20, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("local padding", &local_padding, 0, 20, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("global padding", &global_padding, 0, 80, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    TicTacToe_Ultimate_FEW::TicTacToe_Ultimate_FEW():
        FrontendWrap("TicTacToe_Ultimate")
    {}

    TicTacToe_Ultimate_FEW::~TicTacToe_Ultimate_FEW()
    {}
    
    bool TicTacToe_Ultimate_FEW::game_methods_compatible(const game_methods* methods)
    {
        return (strcmp(methods->game_name, "TicTacToe") == 0 && strcmp(methods->variant_name, "Ultimate") == 0 && strcmp(methods->impl_name, "surena_default") == 0);
    }
    
    Frontend* TicTacToe_Ultimate_FEW::new_frontend()
    {
        return new TicTacToe_Ultimate();
    }

    void TicTacToe_Ultimate_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
