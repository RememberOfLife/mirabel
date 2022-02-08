#pragma once

#include <cstdint>

#include "SDL.h"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "prototype_util/st_gui.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class EmptyFrontend : public Frontend {
        public:
            EmptyFrontend();
            ~EmptyFrontend();
            void set_game(surena::Game* new_game) override;
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
    };

}
