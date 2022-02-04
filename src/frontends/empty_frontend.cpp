#include <cstdint>

#include "SDL.h"

#include "meta_gui/meta_gui.hpp"

#include "frontends/empty_frontend.hpp"

namespace Frontends {

    EmptyFrontend::EmptyFrontend()
    {
        log = MetaGui::log_register("F/empty");
    }

    EmptyFrontend::~EmptyFrontend()
    {
        MetaGui::log_unregister(log);
    }

    void EmptyFrontend::process_event(SDL_Event event)
    {
        MetaGui::logf(log, "SDL event: %d\n", event.type);
    }

    void EmptyFrontend::update()
    {
        
    }

    void EmptyFrontend::render()
    {
        DD::SetRGB(0.45, 0.55, 0.6);
        DD::Clear();
    }

}
