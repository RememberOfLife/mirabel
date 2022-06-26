#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class TwixT_PP : public Frontend {

        private:

            NVGcontext* dc;

            game* the_game;
            twixt_pp_options the_game_opts = (twixt_pp_options) {
                .wx = 24,
                .wy = 24,
            };
            twixt_pp_internal_methods* the_game_int;

            player_id pbuf;
            uint8_t pbuf_c;

            float padding = 40;
            float button_size = 10;

            struct sbtn {
                float x;
                float y;
                float r;
                bool hovered;
                bool mousedown;
                void update(float mx, float my);
            };

            int mx;
            int my;

            sbtn* board_buttons; // board_buttons origin is top left, row major

        public:

            TwixT_PP();
            ~TwixT_PP();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class TwixT_PP_FEW : public FrontendWrap {
        public:
            TwixT_PP_FEW();
            ~TwixT_PP_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
