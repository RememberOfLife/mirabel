#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include "imgui.h"
#include "surena/games/havannah.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "games/game_catalogue.hpp"
#include "games/havannah.hpp"
#include "meta_gui/meta_gui.hpp"
#include "prototype_util/direct_draw.hpp"

#include "frontends/havannah.hpp"

namespace Frontends {

    void Havannah::sbtn::update(float mx, float my) {
        const float hex_angle = 2 * M_PI / 6;
        mx -= x;
        my -= y;
        // mouse is auto rotated by update to make this function assume global flat top
        // rotate in button space to make the collision work with the pointy top local hexes we get from global flat top
        rotate_cords(mx, my, hex_angle);
        mx = abs(mx);
        my = abs(my);
        // https://stackoverflow.com/questions/42903609/function-to-determine-if-point-is-inside-hexagon
        hovered = (my < std::round(sqrt(3) * std::min(r - mx, r / 2)));
    }

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
        const float hex_angle = 2 * M_PI / 6;
        const float fitting_hex_radius = button_size+padding;
        const float flat_radius = sin(hex_angle) * fitting_hex_radius;
        int board_sizer = (2*size-1);
        float offset_x = -(size*flat_radius)+flat_radius;
        float offset_y = -((3*size-3)*fitting_hex_radius)/2;
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < size) || !(y - x < size)) {
                    continue;
                }
                int base_x_padding = (y + 1) / 2;
                float base_x = (x - base_x_padding) * (flat_radius * 2);
                if (y % 2 != 0) {
                    base_x += flat_radius;
                }
                float base_y = y*(fitting_hex_radius*1.5);
                board_buttons[y*board_sizer+x] = sbtn{
                    offset_x + base_x, offset_y + base_y, button_size,
                    static_cast<uint8_t>(x), static_cast<uint8_t>(y),
                    false, false
                };
            }
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
                mx = event.motion.x - x_px;
                my = event.motion.y - y_px;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    const float hex_angle = 2 * M_PI / 6;
                    const float fitting_hex_radius = button_size+padding;
                    const float flat_radius = sin(hex_angle) * fitting_hex_radius;
                    int board_sizer = (2*size-1);
                    float mX = event.button.x - x_px;
                    float mY = event.button.y - y_px;
                    mX -= w_px / 2;
                    mY -= h_px / 2;
                    if (!flat_top) {
                        rotate_cords(mX, mY, hex_angle);
                    }
                    for (int y = 0; y < board_sizer; y++) {
                        for (int x = 0; x < board_sizer; x++) {
                            if (!(x - y < size) || !(y - x < size)) {
                                continue;
                            }
                            board_buttons[y*board_sizer+x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                if (board_buttons[y*board_sizer+x].hovered && board_buttons[y*board_sizer+x].mousedown && game->get_cell(x, y) == 0) {
                                    uint64_t move_code = y | (x<<8);
                                    Control::main_client->inbox.push(Control::event::create_move_event(Control::EVENT_TYPE_GAME_MOVE, move_code));
                                }
                                board_buttons[y*board_sizer+x].mousedown = false;
                            }
                            board_buttons[y*board_sizer+x].mousedown |= (board_buttons[y*board_sizer+x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        }
                    }
                }
            } break;
        }
    }

    void Havannah::update()
    {
        if (!game || game->player_to_move() == 0) {
            return;
        }
        // set button hovered
        const float hex_angle = 2 * M_PI / 6;
        const float fitting_hex_radius = button_size+padding;
        const float flat_radius = sin(hex_angle) * fitting_hex_radius;
        int board_sizer = (2*size-1);
        float offset_x = -(size*flat_radius)+flat_radius;
        float offset_y = -((3*size-3)*fitting_hex_radius)/2;
        float mX = mx;
        float mY = my;
        mX -= w_px/2;
        mY -= h_px/2;
        if (!flat_top) {
            // if global board is not flat topped, rotate the mouse so it is, for the collision check
            rotate_cords(mX, mY, hex_angle);
        }
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < size) || !(y - x < size)) {
                    continue;
                }
                int base_x_padding = (y + 1) / 2;
                float base_x = (x - base_x_padding) * (flat_radius * 2);
                if (y % 2 != 0) {
                    base_x += flat_radius;
                }
                float base_y = y*(fitting_hex_radius*1.5);
                board_buttons[y*board_sizer+x].x = offset_x + base_x;
                board_buttons[y*board_sizer+x].y = offset_y + base_y;
                board_buttons[y*board_sizer+x].r = button_size;
                board_buttons[y*board_sizer+x].update(mX, mY);
            }
        }
    }

    void Havannah::render()
    {
        const float hex_angle = 2 * M_PI / 6;
        const float fitting_hex_radius = button_size+padding;
        const float flat_radius = sin(hex_angle) * fitting_hex_radius;
        int board_sizer = (2*size-1);
        DD::SetRGB255(201, 144, 73);
        DD::Clear();
        DD::Push();
        DD::Translate(w_px/2, h_px/2);
        if (!flat_top) {
            DD::Rotate(hex_angle/2);
        }
        DD::Translate(-(size*flat_radius)+flat_radius, -((3*size-3)*fitting_hex_radius)/2);
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < size) || !(y - x < size)) {
                    continue;
                }
                int base_x_padding = (y + 1) / 2;
                float base_x = (x - base_x_padding) * (flat_radius * 2);
                if (y % 2 != 0) {
                    base_x += flat_radius;
                }
                float base_y = y*(fitting_hex_radius*1.5);
                DD::SetFill();
                if (!game || game->player_to_move() == 0) {
                    DD::SetRGB255(161, 119, 67);
                } else {
                    DD::SetRGB255(240, 217, 181);
                }
                DD::Push();
                DD::Translate(base_x, base_y);
                DD::Rotate(hex_angle/2);
                DD::DrawRegularPolygon(6, 0, 0, button_size);
                if (!game) {
                    DD::Pop();
                    continue;
                }
                switch (game->get_cell(x, y)) {
                    case surena::Havannah::COLOR_NONE: {
                        if (board_buttons[y*board_sizer+x].hovered) {
                            DD::SetRGB255(220, 197, 161);
                            DD::SetFill();
                            DD::DrawRegularPolygon(6, 0, 0, button_size*0.9);
                        }
                    } break;
                    case surena::Havannah::COLOR_WHITE: {
                        DD::SetRGB255(141, 35, 35);
                        DD::SetFill();
                        DD::DrawCircle(0, 0, button_size*0.6);
                    } break;
                    case surena::Havannah::COLOR_BLACK: {
                        DD::SetRGB255(25, 25, 25);
                        DD::SetFill();
                        DD::DrawCircle(0, 0, button_size*0.6);
                    } break;
                    case surena::Havannah::COLOR_INVALID: {
                        assert(false);
                    } break;
                }
                //TODO draw colored connections to same player pieces
                if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((x<<8)|y)) {
                    DD::SetRGB255(125, 187, 248);
                    DD::SetStroke();
                    DD::SetLineWidth(button_size*0.1);
                    DD::DrawRegularPolygon(6, 0, 0, button_size*0.95);
                }
                DD::Pop();
            }
        }
        DD::Pop();
    }

    void Havannah::draw_options()
    {
        ImGui::Checkbox("flat top", &flat_top);
        ImGui::SliderFloat("button size", &button_size, 10, 90, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &padding, 0, 20, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    void Havannah::rotate_cords(float& x, float&y, float angle)
    {
        // rotate mouse input by +angle
        float pr = hypot(x, y);
        float pa = atan2(y, x) + angle;
        y = -pr * cos(pa);
        x = pr * sin(pa);
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
    
    void Havannah_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
