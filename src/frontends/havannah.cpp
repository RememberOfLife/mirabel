#include "cmath"
#include <cstdint>

#include "SDL.h"
#include "imgui.h"
#include "surena/games/havannah.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "games/havannah.hpp"
#include "prototype_util/direct_draw.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/havannah.hpp"

namespace Frontends {

    Havannah::Havannah():
        game(NULL),
        engine(NULL)
    {}

    Havannah::~Havannah()
    {}

    void Havannah::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::Havannah*>(new_game);
        if (game != nullptr) {
            size = game->get_size();
        } else {
            size = 10;
        }
    }

    void Havannah::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    void Havannah::process_event(SDL_Event event)
    {
        if (!game || game->player_to_move() == 0) {
            // if no game, or game is done, don't process anything
            return;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                mx = event.motion.x;
                my = event.motion.y;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                
            } break;
        }
    }

    void Havannah::update()
    {
        if (!game || game->player_to_move() == 0) {
            return;
        }
        
    }

    void Havannah::render()
    {
        const float a = 2 * M_PI / 6;
        const float fitting_hex_radius = button_size+padding;
        const float flat_radius = sin(a) * fitting_hex_radius;
        int board_sizer = (2*size-1);
        DD::SetRGB255(201, 144, 73);
        DD::Clear();
        DD::Push();
        DD::Translate(w_px/2, h_px/2);
        if (!flat_top) {
            DD::Rotate(a/2);
        }
        DD::Translate(-(size*flat_radius)+flat_radius, -((3*size-3)*fitting_hex_radius)/2);
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < size) || !(y - x < size)) {
                    continue;
                }
                float base_x = (x-((y+1)/2))*(flat_radius*2)+(y%2==0 ? 0 : flat_radius);
                float base_y = y*(fitting_hex_radius*1.5);
                DD::SetFill();
                if (!game || game->player_to_move() == 0) {
                    DD::SetRGB255(161, 119, 67);
                } else {
                    DD::SetRGB255(240, 217, 181);
                }
                DD::Push();
                DD::Translate(base_x, base_y);
                DD::Rotate(a/2);
                DD::DrawRegularPolygon(6, 0, 0, button_size);
                DD::Pop();
            }
        }
        DD::Pop();
        DD::SetLineWidth(1);
        DD::SetRGB255(0, 255, 0);
        DD::DrawLine(w_px/2, 0, w_px/2, h_px);
        DD::DrawLine(0, h_px/2, w_px, h_px/2);
    }

    void Havannah::draw_options()
    {
        ImGui::Checkbox("flat top", &flat_top);
        ImGui::SliderFloat("button size", &button_size, 10, 60, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &padding, 0, 15, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    Havannah_FEW::Havannah_FEW():
        FrontendWrap("Havannah")
    {}

    Havannah_FEW::~Havannah_FEW()
    {}
    
    bool Havannah_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::Havannah*>(base_game_variant) != nullptr);
    }
    
    Frontend* Havannah_FEW::new_frontend()
    {
        return new Havannah();
    }

}
