#include <cstdint>

#include "SDL.h"

#include "meta_gui/meta_gui.hpp"

#include "games/default_ctx.hpp"

namespace Games {

    DefaultCtx::DefaultCtx()
    {
        log = MetaGui::log_register("DefaultCtx");
        MetaGui::log("created default context\n");
    }

    DefaultCtx::~DefaultCtx()
    {
        MetaGui::log_unregister(log);
    }

    void DefaultCtx::process_event(SDL_Event event)
    {
        MetaGui::logf(log, "SDL event: %d\n", event.type);
    }

    void DefaultCtx::update()
    {
        
    }

    void DefaultCtx::render()
    {
        DD::SetRGB(0.45, 0.55, 0.6);
        DD::Clear();
    }

}
