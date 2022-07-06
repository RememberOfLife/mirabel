#pragma once

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class EmptyFrontend : public Frontend {
        private:
            NVGcontext* dc;
            char vstr[64];
        public:
            EmptyFrontend();
            ~EmptyFrontend();
            void set_game(game* new_game) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;
    };

    class EmptyFrontend_FEW : public FrontendWrap {
        public:
            EmptyFrontend_FEW();
            ~EmptyFrontend_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
