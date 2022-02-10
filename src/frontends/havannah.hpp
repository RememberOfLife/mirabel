#pragma once

#include "SDL.h"
#include "surena/games/havannah.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class Havannah : public Frontend {

        public:

            surena::Havannah* game;
            surena::Engine* engine;

            int size;

            bool flat_top = false;
            float button_size = 25;
            float padding = 2.5;

            int mx;
            int my;

            Havannah();
            ~Havannah();
            void set_game(surena::Game* new_game) override;
            void set_engine(surena::Engine* new_engine) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

    };

    class Havannah_FEW : public FrontendWrap {
        public:
            Havannah_FEW();
            ~Havannah_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
    };

}