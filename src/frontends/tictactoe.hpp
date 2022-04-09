#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "surena/games/tictactoe.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class TicTacToe : public Frontend {

        private:

            NVGcontext* dc;

            surena::TicTacToe* game;
            surena::Engine* engine;

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
            void set_game(surena::Game* new_game) override;
            void set_engine(surena::Engine* new_engine) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class TicTacToe_FEW : public FrontendWrap {
        public:
            TicTacToe_FEW();
            ~TicTacToe_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
