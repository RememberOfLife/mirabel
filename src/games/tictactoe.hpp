#pragma once

#include <cstdint>

#include "SDL.h"

#include "prototype_util/st_gui.hpp"

#include "state_control/drawing_context_app.hpp"

namespace Games {

    class TicTacToe : public DrawingContextApp {

        private:

            uint32_t log;
            
            /*
            board as:
            789
            456
            123
            */
            uint8_t board_state[3][3];
            STGui::btn_rect board_buttons[3][3];
            uint8_t player_to_move = 1;

        public:

            TicTacToe();

            ~TicTacToe();

            void process_event(SDL_Event event) override;

            void update() override;

            void render() override;

    };

}
