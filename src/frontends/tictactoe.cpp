#include <cstdint>

#include <SDL2/SDL.h>
#include "imgui.h"
#include "surena/games/tictactoe.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "games/game_catalogue.hpp"
#include "games/tictactoe.hpp"
#include "prototype_util/direct_draw.hpp"

#include "frontends/tictactoe.hpp"

namespace Frontends {

    void TicTacToe::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    }

    TicTacToe::TicTacToe():
        game(NULL),
        engine(NULL)
    {
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x] = sbtn{
                    static_cast<float>(x)*(button_size+padding),
                    (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding),
                    button_size, button_size, false, false
                };
            }
        }
    }

    TicTacToe::~TicTacToe()
    {}

    void TicTacToe::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::TicTacToe*>(new_game);
    }

    void TicTacToe::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    void TicTacToe::process_event(SDL_Event event)
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
                    mX -= w_px/2-(3*button_size+2*padding)/2;
                    mY -= h_px/2-(3*button_size+2*padding)/2;
                    for (int x = 0; x < 3; x++) {
                        for (int y = 0; y < 3; y++) {
                            board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                if (board_buttons[y][x].hovered && board_buttons[y][x].mousedown && game->get_cell(x, y) == 0) {
                                    uint64_t move_code = x | (y<<2);
                                    Control::main_client->inbox.push(Control::event::create_move_event(Control::EVENT_TYPE_GAME_MOVE, move_code));
                                }
                                board_buttons[y][x].mousedown = false;
                            }
                            board_buttons[y][x].mousedown |= (board_buttons[y][x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        }
                    }
                }
            } break;
        }
    }

    void TicTacToe::update()
    {
        if (!game || game->player_to_move() == 0) {
            return;
        }
        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(3*button_size+2*padding)/2;
        mY -= h_px/2-(3*button_size+2*padding)/2;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x].x = static_cast<float>(x)*(button_size+padding);
                board_buttons[y][x].y = (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding);
                board_buttons[y][x].w = button_size;
                board_buttons[y][x].h = button_size;
                board_buttons[y][x].update(mX, mY);
            }
        }
    }

    void TicTacToe::render()
    {
        DD::SetLineWidth(button_size*0.175);
        DD::SetRGB255(201, 144, 73);
        DD::Clear();
        DD::Push();
        DD::Translate(w_px/2-(3*button_size+2*padding)/2, h_px/2-(3*button_size+2*padding)/2);
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                float base_x = static_cast<float>(x)*(button_size+padding);
                float base_y = (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding);
                DD::SetFill();
                if (!game || game->player_to_move() == 0) {
                    DD::SetRGB255(161, 119, 67);
                } else {
                    DD::SetRGB255(240, 217, 181);
                }
                DD::DrawRectangle(base_x, base_y, button_size, button_size);
                if (!game) {
                    continue;
                }
                uint8_t player_in_cell = game->get_cell(x, y);
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
                } else if (board_buttons[y][x].hovered && game->player_to_move() > surena::TicTacToe::PLAYER_NONE) {
                    DD::SetRGB255(220, 197, 161);
                    DD::SetFill();
                    DD::DrawRectangle(board_buttons[y][x].x+button_size*0.05, board_buttons[y][x].y+button_size*0.05, board_buttons[y][x].w-button_size*0.1, board_buttons[y][x].h-button_size*0.1);
                }
                if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((y<<2)|x)) {
                    DD::SetRGB255(125, 187, 248);
                    DD::SetFill();
                    DD::DrawCircle(board_buttons[y][x].x+button_size/2, board_buttons[y][x].y+button_size/2, button_size*0.15);
                }
            }
        }
        DD::Pop();
    }

    void TicTacToe::draw_options()
    {
        ImGui::SliderFloat("button size", &button_size, 20, 400, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &padding, 0, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    TicTacToe_FEW::TicTacToe_FEW():
        FrontendWrap("TicTacToe")
    {}

    TicTacToe_FEW::~TicTacToe_FEW()
    {}
    
    bool TicTacToe_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::TicTacToe*>(base_game_variant) != nullptr);
    }
    
    Frontend* TicTacToe_FEW::new_frontend()
    {
        return new TicTacToe();
    }

    void TicTacToe_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
