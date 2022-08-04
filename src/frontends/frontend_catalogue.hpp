#pragma once

#include <vector>

#include <SDL2/SDL.h>
#include "surena/game.h"

#include "games/game_catalogue.hpp"

namespace Frontends {

    class Frontend {

        public:

            float w_px;
            float h_px;
            float x_px;
            float y_px;

            virtual ~Frontend() = default;

            // set internal game pointer of the frontend if compatible, otherwise NULL
            // pass NULL to this function to unset the game
            // any data passed previously by this function, must be assumed invalid when called again
            virtual void set_game(game* new_game) = 0;

            // set internal engine pointer, pass NULL to unset
            // any data passed previously by this function, must be assumed invalid when called again
            // virtual void set_engine(surena::Engine* new_engine) = 0;

            virtual void process_event(SDL_Event event) = 0;

            virtual void update() = 0;

            virtual void render() = 0;

            virtual void draw_options() = 0; // draw options available to configure the frontend, e.g. colors and sizes
            
    };

    class FrontendWrap {
        public:
            const char* name;
            FrontendWrap(const char* name);
            virtual ~FrontendWrap() = default;
            virtual bool game_methods_compatible(const game_methods* methods) = 0;
            virtual Frontend* new_frontend() = 0;
            virtual void draw_options() = 0; // draw options that need to be set pre frontend creation
    };

    extern std::vector<FrontendWrap*> frontend_catalogue;

} // namespace Frontends
