#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "surena/games/chess.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class Chess : public Frontend {

        private:

            surena::Chess* game;
            surena::Engine* engine;

            float square_size = 90;

        public:

            Chess();
            ~Chess();
            void set_game(surena::Game* new_game) override;
            void set_engine(surena::Engine* new_engine) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render(NVGcontext* ctx) override;
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
