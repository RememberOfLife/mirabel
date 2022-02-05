#pragma once

#include <vector>

#include "SDL.h"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

namespace Frontends {

    class Frontend {

        public:

            surena::PerfectInformationGame* game;

            float w_px;
            float h_px;

            virtual ~Frontend() = default;

            //TODO method to pass game pointer so the frontend can check compatibility itself

            virtual void process_event(SDL_Event event) = 0;

            virtual void update() = 0; //TODO maybe this should receive the latest mouse position, for hover effects, so not every frontend has to listen to all mouse move events just for that, if so then collect mouse state in guithread

            virtual void render() = 0;

            virtual void draw_options() = 0; // draw options available to configure the frontend, e.g. colors and sizes
            
    };

    class FrontendWrap {
        public:
            const char* name;
            FrontendWrap(const char* name);
            virtual ~FrontendWrap() = default;
            virtual bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) = 0;
            virtual Frontend* new_frontend() = 0;
            //TODO if required put any options that need to be set pre frontend creation here in a draw_options
    };

    extern std::vector<FrontendWrap*> frontend_catalogue;

} // namespace Frontends
