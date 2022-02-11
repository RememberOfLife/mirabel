#include <cstdint>

#include "SDL.h"
#include "imgui.h"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"
#include "prototype_util/direct_draw.hpp"

#include "frontends/empty_frontend.hpp"

namespace Frontends {

    EmptyFrontend::EmptyFrontend()
    {}

    EmptyFrontend::~EmptyFrontend()
    {}

    void EmptyFrontend::set_game(surena::Game* game)
    {}

    void EmptyFrontend::set_engine(surena::Engine* new_engine)
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
        return false; // empty frontend will never be listed explicitly, but can always be used as universal unloader by loading it instead
    }
    
    Frontend* EmptyFrontend_FEW::new_frontend()
    {
        return new EmptyFrontend();
    }

    void EmptyFrontend_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
