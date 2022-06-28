#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "surena/games/chess.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class Chess : public Frontend {

        //TODO display when you king is in check
        //TODO draw arrows and markers with right click
        //TODO highlist last made mode

        private:

            NVGcontext* dc;

            game* the_game;
            chess_internal_methods* the_game_int;

            player_id pbuf;
            uint8_t pbuf_c;
            move_code moves[CHESS_MAX_MOVES];
            uint32_t move_cnt;

            float square_size = 90;
            bool auto_size = true;
            bool promotion_auto_queen = false;

            int sprites[12];

            struct sbtn {
                float x;
                float y;
                float s;
                bool hovered;
                void update(float mx, float my);
            };

            int mx;
            int my;

            sbtn board_buttons[8][8]; // board_buttons[y][x] origin is bottom left

            std::unordered_map<uint8_t, std::vector<uint8_t>> move_map;
            bool passive_pin = false;
            bool hover_outside_of_pin = false;
            int mouse_pindx_x = -1;
            int mouse_pindx_y = -1;

            sbtn promotion_buttons[2][2];
            bool promotion_buttons_mdown = false;
            int promotion_ox = -1;
            int promotion_oy = -1;
            int promotion_tx = -1;
            int promotion_ty = -1;

        public:

            Chess();
            ~Chess();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class Chess_FEW : public FrontendWrap {
        public:
            Chess_FEW();
            ~Chess_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
