#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/havannah.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "games/game_catalogue.hpp"
#include "games/havannah.hpp"
#include "meta_gui/meta_gui.hpp"

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
        the_game(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
    }

    Havannah::~Havannah()
    {}

    void Havannah::set_game(game* new_game)
    {
        the_game = new_game;
        if (the_game != NULL) {
            the_game_int = (havannah_internal_methods*)the_game->methods->internal_methods;
            the_game_int->get_size(the_game, &size);
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

    void Havannah::process_event(SDL_Event event)
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
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
                                HAVANNAH_PLAYER cp;
                                the_game_int->get_cell(the_game, x, y, &cp);
                                if (board_buttons[y*board_sizer+x].hovered && board_buttons[y*board_sizer+x].mousedown && cp == 0) {
                                    uint64_t move_code = y | (x<<8);
                                    f_event_any es;
                                    f_event_create_game_move(&es, move_code);
                                    f_event_queue_push(&Control::main_client->inbox, &es);
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
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
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
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2, h_px/2);
        if (!flat_top) {
            nvgRotate(dc, hex_angle/2);
        }
        // colored board border for current/winning player
        pbuf = HAVANNAH_PLAYER_NONE;
        nvgBeginPath(dc);
        if (!the_game) {
            nvgStrokeColor(dc, nvgRGB(161, 119, 67));
        } else {
            the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
            if (pbuf_c == 0) {
                the_game->methods->get_results(the_game, &pbuf_c, &pbuf);
            }
            switch (pbuf) {
                default:
                case HAVANNAH_PLAYER_NONE: {
                    nvgStrokeColor(dc, nvgRGB(128, 128, 128));
                } break;
                case HAVANNAH_PLAYER_WHITE: {
                    nvgStrokeColor(dc, nvgRGB(141, 35, 35));
                } break;
                case HAVANNAH_PLAYER_BLACK: {
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                } break;
            }
            // actually we just want the ptm in there, so reste it back to that
            the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        }
        nvgStrokeWidth(dc, flat_radius*0.5);
        nvgMoveTo(dc, static_cast<float>(size*2)*flat_radius, 0);
        for (int i = 0; i < 6; i++) {
            nvgRotate(dc, M_PI/3);
            nvgLineTo(dc, static_cast<float>(size*2)*flat_radius, 0);
        }
        nvgStroke(dc);
        // translate back up to board rendering position and render board
        nvgTranslate(dc, -(size*flat_radius)+flat_radius, -((3*size-3)*fitting_hex_radius)/2);
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
                nvgBeginPath(dc);
                if (!the_game || pbuf_c == 0) {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                } else {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                }
                nvgSave(dc);
                nvgTranslate(dc, base_x, base_y);
                nvgRotate(dc, hex_angle/2);
                nvgMoveTo(dc, button_size, 0);
                for (int i = 0; i < 6; i++) {
                    nvgRotate(dc, M_PI/3);
                    nvgLineTo(dc, button_size, 0);
                }
                nvgFill(dc);
                if (!the_game) {
                    nvgRestore(dc);
                    continue;
                }
                HAVANNAH_PLAYER cell_color;
                the_game_int->get_cell(the_game, x, y, &cell_color);
                switch (cell_color) {
                    case HAVANNAH_PLAYER_NONE: {
                        if (board_buttons[y*board_sizer+x].hovered && pbuf > HAVANNAH_PLAYER_NONE) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(220, 197, 161));
                            nvgMoveTo(dc, button_size*0.9, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI/3);
                                nvgLineTo(dc, button_size*0.9, 0);
                            }
                            nvgFill(dc);
                        }
                    } break;
                    case HAVANNAH_PLAYER_WHITE: {
                        nvgBeginPath(dc);
                        nvgFillColor(dc, nvgRGB(141, 35, 35));
                        if (hex_stones) {
                            nvgMoveTo(dc, button_size*stone_size_mult, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI/3);
                                nvgLineTo(dc, button_size*stone_size_mult, 0);
                            }
                        } else {
                            nvgCircle(dc, 0, 0, button_size*stone_size_mult);
                        }
                        nvgFill(dc);
                    } break;
                    case HAVANNAH_PLAYER_BLACK: {
                        nvgBeginPath(dc);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        if (hex_stones) {
                            nvgMoveTo(dc, button_size*stone_size_mult, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI/3);
                                nvgLineTo(dc, button_size*stone_size_mult, 0);
                            }
                        } else {
                            nvgCircle(dc, 0, 0, button_size*stone_size_mult);
                        }
                        nvgFill(dc);
                    } break;
                    case HAVANNAH_PLAYER_INVALID: {
                        assert(false);
                    } break;
                }
                // draw colored connections to same player pieces
                if (connections_width > 0 && cell_color != HAVANNAH_PLAYER_NONE) {
                    float connection_draw_width = button_size * connections_width;
                    // draw for self<->{1(x-1,y),3(x,y-1),2(x-1,y-1)}
                    uint8_t connections_to_draw = 0;
                    HAVANNAH_PLAYER cell_other;
                    the_game_int->get_cell(the_game, x-1, y, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b001;
                    }
                    the_game_int->get_cell(the_game, x, y-1, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b100;
                    }
                    the_game_int->get_cell(the_game, x-1, y-1, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b010;
                    }
                    if (connections_to_draw) {
                        nvgSave(dc);
                        nvgRotate(dc, -M_PI/6);
                        nvgBeginPath(dc);
                        switch (cell_color) {
                            case HAVANNAH_PLAYER_WHITE: {
                                nvgStrokeColor(dc, nvgRGB(141, 35, 35));
                            } break;
                            case HAVANNAH_PLAYER_BLACK: {
                                nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                            } break;
                            case HAVANNAH_PLAYER_NONE:
                            case HAVANNAH_PLAYER_INVALID: {
                                assert(false);
                            } break;
                        }
                        nvgRotate(dc, -M_PI-M_PI/3);
                        for (int rot = 0; rot < 3; rot++) {
                            nvgRotate(dc, M_PI/3);
                            if (!((connections_to_draw >> rot)&0b1)) {
                                continue;
                            }
                            nvgRect(dc, -connection_draw_width/2, -connection_draw_width/2, connection_draw_width+flat_radius*2, connection_draw_width);
                        }
                        nvgFill(dc);
                        nvgRestore(dc);
                    }
                }
                //TODO draw engine best move 
                /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((x<<8)|y)) {
                    nvgStrokeColor(dc, nvgRGB(125, 187, 248));
                    nvgStrokeWidth(dc, button_size*0.1);
                    nvgMoveTo(dc, button_size*0.95, 0);
                    for (int i = 0; i < 6; i++) {
                        nvgRotate(dc, M_PI/3);
                        nvgLineTo(dc, button_size*0.95, 0);
                    }
                    nvgStroke(dc);
                }*/
                nvgRestore(dc);
            }
        }
        nvgRestore(dc);
    }

    void Havannah::draw_options()
    {
        ImGui::Checkbox("flat top", &flat_top);
        ImGui::SliderFloat("button size", &button_size, 10, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &padding, 0, 20, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("stone size", &stone_size_mult, 0.1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::Checkbox("hex stones", &hex_stones);
        ImGui::SliderFloat("connections width", &connections_width, 0, 0.8, "%.3f", ImGuiSliderFlags_AlwaysClamp);
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
