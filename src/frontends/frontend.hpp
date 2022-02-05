#pragma once

#include "SDL.h"
#include "surena/game.hpp"

namespace Frontends {

    class Frontend {

        public:

            surena::PerfectInformationGame* game;

            float w_px;
            float h_px;

            virtual ~Frontend() = default;

            virtual void process_event(SDL_Event event) = 0;

            virtual void update() = 0; //TODO maybe this should receive the latest mouse position, for hover effects, so not every frontend has to listen to all mouse move events just for that, if so then collect mouse state in guithread

            virtual void render() = 0;
            
    };

} // namespace Frontends
