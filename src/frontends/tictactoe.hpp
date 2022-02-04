#pragma once

#include <cstdint>

#include "SDL.h"

#include "frontends/frontend.hpp"

namespace Frontends {

    class TicTacToe : public Frontend {

        private:

            uint32_t log;

            struct sbtn {
                float x;
                float y;
                float w;
                float h;
                bool hovered;
                bool mousedown;
                void update(float mx, float my);
            };

            int mx;
            int my;

            //TODO size and color variables for guistate config
            sbtn board_buttons[3][3]; // board_buttons[y][x] origin is bottom left

        public:

            TicTacToe();

            ~TicTacToe();

            void process_event(SDL_Event event) override;

            void update() override;

            void render() override;

    };

}
