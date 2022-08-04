#pragma once

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "surena/games/tictactoe.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class TicTacToe : public Frontend {

        private:

            NVGcontext* dc;

            game* the_game;
            tictactoe_internal_methods* the_game_int;

            player_id pbuf;
            uint8_t pbuf_c;

            float button_size = 200;
            float padding = 20;

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

            sbtn board_buttons[3][3]; // board_buttons[y][x] origin is bottom left

        public:

            TicTacToe();
            ~TicTacToe();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class TicTacToe_FEW : public FrontendWrap {
        public:
            TicTacToe_FEW();
            ~TicTacToe_FEW();
            bool game_methods_compatible(const game_methods* methods) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
