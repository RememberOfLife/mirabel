#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/chess.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "games/chess.hpp"
#include "prototype_util/direct_draw.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/chess.hpp"

namespace Frontends {

    Chess::Chess():
        game(NULL),
        engine(NULL)
    {
        
    }

    Chess::~Chess()
    {}

    void Chess::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::Chess*>(new_game);
    }

    void Chess::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    void Chess::process_event(SDL_Event event)
    {
        
    }

    void Chess::update()
    {
        
    }

    void Chess::render(NVGcontext* ctx)
    {
        
    }

    void Chess::draw_options()
    {
        ImGui::SliderFloat("button size", &square_size, 48, 250, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    Chess_FEW::Chess_FEW():
        FrontendWrap("Chess")
    {}

    Chess_FEW::~Chess_FEW()
    {}
    
    bool Chess_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::Chess*>(base_game_variant) != nullptr);
    }
    
    Frontend* Chess_FEW::new_frontend()
    {
        return new Chess();
    }

    void Chess_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
