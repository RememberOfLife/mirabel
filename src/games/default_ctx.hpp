#pragma once

#include <cstdint>

#include "SDL.h"

#include "prototype_util/st_gui.hpp"

#include "state_control/drawing_context_app.hpp"

namespace Games {

    class DefaultCtx : public DrawingContextApp {

        private:

            uint32_t log;

        public:

            DefaultCtx();

            ~DefaultCtx();

            void process_event(SDL_Event event) override;

            void update() override;

            void render() override;

    };

}
