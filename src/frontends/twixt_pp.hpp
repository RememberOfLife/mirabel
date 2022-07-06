#pragma once

#include <SDL2/SDL.h>
#include "nanovg.h"
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

            float padding = 38;
            float button_size = 10;
            bool auto_size = true;
            bool display_analysis_background = true;
            bool display_hover_indicator_cross = false;
            bool display_hover_connections = true;
            bool display_runoff_lines = true;
            bool display_rankfile = true;

            bool rankfile_yoffset = true;

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
            bool swap_hover = false;
            bool swap_down = false;

            int hover_rank;
            int hover_file;

        public:

            TwixT_PP();
            ~TwixT_PP();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

            void draw_dashed_line(float x1, float y1, float x2, float y2, float w, float g, NVGcolor col);
            void draw_cond_connection(float bx, float by, uint8_t x, uint8_t y, TWIXT_PP_DIR d, NVGcolor ccol);

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
