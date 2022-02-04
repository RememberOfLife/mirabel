#pragma once

#include <cstdint>

#include "SDL.h"

#include "prototype_util/st_gui.hpp"

#include "frontends/frontend.hpp"

namespace Frontends {

    class EmptyFrontend : public Frontend {

        private:

            uint32_t log;

        public:

            EmptyFrontend();

            ~EmptyFrontend();

            void process_event(SDL_Event event) override;

            void update() override;

            void render() override;

    };

}
