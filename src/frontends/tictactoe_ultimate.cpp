#include <cstdint>

#include <SDL2/SDL.h>
#include "imgui.h"
#include "surena/games/tictactoe_ultimate.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "games/tictactoe_ultimate.hpp"
#include "prototype_util/direct_draw.hpp"
#include "state_control/client.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/tictactoe_ultimate.hpp"

namespace Frontends {

    void TicTacToe_Ultimate::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    }

    TicTacToe_Ultimate::TicTacToe_Ultimate():
        game(NULL),
        engine(NULL)
    {
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

    void TicTacToe_Ultimate::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::TicTacToe_Ultimate*>(new_game);
    }

    void TicTacToe_Ultimate::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    void TicTacToe_Ultimate::process_event(SDL_Event event)
    {
        if (!game || game->player_to_move() == 0) {
            // if no game, or game is done, don't process anything
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
                    uint8_t global_target = game->get_global_target();
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
                                        if (board_buttons[iy][ix].hovered && board_buttons[iy][ix].mousedown && game->get_cell_local(ix, iy) == 0) {
                                            uint64_t move_code = ix | (iy<<4);
                                            StateControl::main_client->t_gui.inbox.push(StateControl::event::create_move_event(StateControl::EVENT_TYPE_GAME_MOVE, move_code));
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
        if (!game || game->player_to_move() == 0) {
            return;
        }
        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
        mY -= h_px/2-(9*button_size+6*local_padding+2*global_padding)/2;
        uint8_t global_target = game->get_global_target();
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
        uint8_t global_target = game ? game->get_global_target() : 0;
        DD::SetRGB255(201, 144, 73);
        DD::Clear();
        DD::Push();
        DD::Translate(w_px/2-(3*local_baord_size+2*global_padding)/2, h_px/2-(3*local_baord_size+2*global_padding)/2);
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                uint8_t local_result = (game ? game->get_cell_global(gx, 2-gy) : 0);
                float base_x = gx*(local_baord_size+global_padding);
                float base_y = gy*(local_baord_size+global_padding);
                if (local_result > 0) {
                    DD::SetLineWidth(local_baord_size*0.175);
                    DD::SetFill();
                    DD::SetRGB255(161, 119, 67);
                    DD::DrawRectangle(base_x, base_y, local_baord_size, local_baord_size);
                    switch (local_result) {
                        case 1: {
                            // X
                            DD::SetStroke();
                            DD::SetRGB255(25, 25, 25);
                            DD::DrawLine(base_x+local_baord_size*0.175, base_y+local_baord_size*0.175, base_x+local_baord_size*0.825, base_y+local_baord_size*0.825);
                            DD::DrawLine(base_x+local_baord_size*0.175, base_y+local_baord_size*0.825, base_x+local_baord_size*0.825, base_y+local_baord_size*0.175);
                        } break;
                        case 2: {
                            // O
                            DD::SetStroke();
                            DD::SetRGB255(25, 25, 25);
                            DD::DrawCircle(base_x+local_baord_size/2, base_y+local_baord_size/2, local_baord_size*0.3);
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
                        DD::SetLineWidth(button_size*0.175);
                        DD::SetFill();
                        if (game && game->player_to_move() != 0 && (global_target == (((2-gy)<<2)|gx) || global_target == ((3<<2)|3))) {
                            DD::SetRGB255(240, 217, 181);
                        } else {
                            DD::SetRGB255(161, 119, 67);
                        }
                        DD::DrawRectangle(base_x, base_y, button_size, button_size);
                        if (!game) {
                            continue;
                        }
                        int ix = gx*3+lx;
                        int iy = 8-(gy*3+ly);
                        uint8_t player_in_cell = game->get_cell_local(ix, iy);
                        if (player_in_cell == 1) {
                            // X
                            DD::SetStroke();
                            DD::SetRGB255(25, 25, 25);
                            DD::DrawLine(base_x+button_size*0.175, base_y+button_size*0.175, base_x+button_size*0.825, base_y+button_size*0.825);
                            DD::DrawLine(base_x+button_size*0.175, base_y+button_size*0.825, base_x+button_size*0.825, base_y+button_size*0.175);
                        } else if (player_in_cell == 2) {
                            // O
                            DD::SetStroke();
                            DD::SetRGB255(25, 25, 25);
                            DD::DrawCircle(base_x+button_size/2, base_y+button_size/2, button_size*0.3);
                        } else if (board_buttons[iy][ix].hovered) {
                            DD::SetRGB255(220, 197, 161);
                            DD::SetFill();
                            DD::DrawRectangle(board_buttons[iy][ix].x+button_size*0.05, board_buttons[iy][ix].y+button_size*0.05, board_buttons[iy][ix].w-button_size*0.1, board_buttons[iy][ix].h-button_size*0.1);
                        }
                        if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((iy<<4)|ix)) {
                            DD::SetRGB255(125, 187, 248);
                            DD::SetFill();
                            DD::DrawCircle(base_x+button_size/2, base_y+button_size/2, button_size*0.15);
                        }
                    }
                }
            }
        }
        DD::Pop();
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
    
    bool TicTacToe_Ultimate_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::TicTacToe_Ultimate*>(base_game_variant) != nullptr);
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
