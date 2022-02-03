#pragma once

#include "SDL.h"
#include "surena_game.hpp"

namespace Games {

    class DrawingContextApp {

        public:

            surena::PerfectInformationGame* game;

            float w_px;
            float h_px;

            virtual ~DrawingContextApp() = default;

            virtual void process_event(SDL_Event event) = 0;

            virtual void update() = 0;

            virtual void render() = 0;
            
    };

} // namespace Games
