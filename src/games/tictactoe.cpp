#include <cstdint>

#include "SDL.h"
#include "surena_game.hpp"
#include "surena_tictactoe.hpp"

#include "meta_gui/meta_gui.hpp"
#include "prototype_util/direct_draw.hpp"
#include "prototype_util/st_gui.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "games/tictactoe.hpp"

namespace Games {

    TicTacToe::TicTacToe()
    {
        log = MetaGui::log_register("TicTacToe");
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_state[y][x] = 0;
                board_buttons[y][x] = STGui::btn_rect{
                    static_cast<float>(x)*250+10, 500-static_cast<float>(y)*250+10, 180, 180,
                    DD::Color4i{240, 217, 181, 0xFF}, //TODO fix for alpha 0
                    DD::Color4i{0, 0, 0, 20},
                    DD::Color4i{0, 0, 0, 20},
                };
            }
        }
        MetaGui::log("created TicTacToe\n");
    }

    TicTacToe::~TicTacToe()
    {
        MetaGui::log_unregister(log);
    }

    void TicTacToe::process_event(SDL_Event event)
    {
        //TODO only detect click on button up, ideally only if the click even started on this tile
    }

    void TicTacToe::update()
    {
        if (game->player_to_move() == 0) {
            return;
        }
        //TODO should mouse really be handled here?! => no STUPID
        int mX;
        int mY;
        uint32_t mouse_buttons = SDL_GetMouseState(&mX, &mY);
        mX -= w_px/2-350;
        mY -= h_px/2-350;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x].update(mX, mY, mouse_buttons);
                if (board_buttons[y][x].is_clicked && board_state[y][x] == 0) {
                    MetaGui::logf(log, "move: player %d (%d,%d)\n", player_to_move, x, y);
                    uint64_t move_code = x | (y<<2);
                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_MOVE, 1, reinterpret_cast<void*>(move_code)));
                    board_state[y][x] = player_to_move;
                    player_to_move = (player_to_move == 1) ? 2 : 1;
                }
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
                } else {
                    board_buttons[y][x].render();
                }
            }
        }
        DD::Pop();
    }

}
