#include <cstdint>

#include "SDL.h"
#include "surena_game.hpp"
#include "surena_tictactoe.hpp"

#include "meta_gui/meta_gui.hpp"
#include "prototype_util/direct_draw.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/tictactoe.hpp"

namespace Frontends {

    void TicTacToe::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    }

    TicTacToe::TicTacToe()
    {
        log = MetaGui::log_register("F/tictactoe");
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x] = sbtn{static_cast<float>(x)*250, 500-static_cast<float>(y)*250, 200, 200, false, false};
            }
        }
    }

    TicTacToe::~TicTacToe()
    {
        MetaGui::log_unregister(log);
    }

    void TicTacToe::process_event(SDL_Event event)
    {
        if (!game || game->player_to_move() == 0) {
            // if no game, or game is done, don't process anything
            return;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                mx = event.motion.x;
                my = event.motion.y;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x;
                    int mY = event.button.y;
                    mX -= w_px/2-350;
                    mY -= h_px/2-350;
                    for (int x = 0; x < 3; x++) {
                        for (int y = 0; y < 3; y++) {
                            board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                if (board_buttons[y][x].hovered && board_buttons[y][x].mousedown && reinterpret_cast<surena::TicTacToe*>(game)->get_cell(x, y) == 0) {
                                    MetaGui::logf(log, "move: player %d (%d,%d)\n", reinterpret_cast<surena::TicTacToe*>(game)->player_to_move(), x, y);
                                    uint64_t move_code = x | (y<<2);
                                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_GAME_MOVE, 1, reinterpret_cast<void*>(move_code)));
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
        mX -= w_px/2-350;
        mY -= h_px/2-350;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x].update(mX, mY);
            }
        }
    }

    void TicTacToe::render()
    {
        DD::SetLineWidth(35);
        DD::SetRGB255(201, 144, 73);
        DD::Clear();
        DD::Push();
        DD::Translate(w_px/2-350, h_px/2-350);
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                float base_x = static_cast<float>(x)*250;
                float base_y = 500-static_cast<float>(y)*250;
                DD::SetFill();
                DD::SetRGB255(240, 217, 181);
                DD::DrawRectangle(base_x, base_y, 200, 200);
                if (!game) {
                    continue;
                }
                uint8_t player_in_cell = reinterpret_cast<surena::TicTacToe*>(game)->get_cell(x, y);
                if (player_in_cell == 1) {
                    // X
                    DD::SetStroke();
                    DD::SetRGB255(25, 25, 25);
                    DD::DrawLine(base_x+35, base_y+35, base_x+165, base_y+165);
                    DD::DrawLine(base_x+35, base_y+165, base_x+165, base_y+35);
                } else if (player_in_cell == 2) {
                    // O
                    DD::SetStroke();
                    DD::SetRGB255(25, 25, 25);
                    DD::DrawCircle(base_x+100, base_y+100, 60);
                } else if (board_buttons[y][x].hovered) {
                    DD::SetRGB255(220, 197, 161);
                    DD::SetFill();
                    DD::DrawRectangle(board_buttons[y][x].x+10, board_buttons[y][x].y+10, board_buttons[y][x].w-20, board_buttons[y][x].h-20);
                }
            }
        }
        DD::Pop();
    }

}
