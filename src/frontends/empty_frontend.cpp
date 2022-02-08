#include <cstdint>

#include "SDL.h"
#include "imgui.h"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "frontends/empty_frontend.hpp"

namespace Frontends {

    EmptyFrontend::EmptyFrontend()
    {}

    EmptyFrontend::~EmptyFrontend()
    {}

    void EmptyFrontend::set_game(surena::Game* game)
    {}

    void EmptyFrontend::process_event(SDL_Event event)
    {}

    void EmptyFrontend::update()
    {}

    void EmptyFrontend::render()
    {
        DD::SetRGB(0.45, 0.55, 0.6);
        DD::Clear();
    }

    void EmptyFrontend::draw_options()
    {
        //TODO make an option for the background color
        ImGui::TextDisabled("<no options>");
    }

    EmptyFrontend_FEW::EmptyFrontend_FEW():
        FrontendWrap("<empty>")
    {}

    EmptyFrontend_FEW::~EmptyFrontend_FEW()
    {}
    
    bool EmptyFrontend_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        //TODO decide if this should really be true, or if there is another unload mechanism
        return true; // empty frontend is compatible with all others, serves as universal unloader
    }
    
    Frontend* EmptyFrontend_FEW::new_frontend()
    {
        return new EmptyFrontend();
    }

}
