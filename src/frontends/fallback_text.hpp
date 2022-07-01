#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class FallbackText : public Frontend {

        private:

            NVGcontext* dc;

            game* the_game;
            uint64_t the_game_step = 0;

            player_id* pbuf = NULL;
            uint8_t pbuf_c = 0;
            player_id* rbuf = NULL;
            uint8_t rbuf_c = 0;

            char* opts_str = NULL;
            char* state_str = NULL;
            uint64_t the_id;
            char* print_str = NULL;

        public:

            FallbackText();
            ~FallbackText();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class FallbackText_FEW : public FrontendWrap {
        public:
            FallbackText_FEW();
            ~FallbackText_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
