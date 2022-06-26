#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "games/game_catalogue.hpp"
#include "games/twixt_pp.hpp"

#include "frontends/twixt_pp.hpp"

namespace Frontends {

    void TwixT_PP::sbtn::update(float mx, float my) {
        hovered = (sqrt( (x - mx) * (x - mx) + (y - my) * (y - my) ) < r);
    }

    TwixT_PP::TwixT_PP():
        the_game(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
        board_buttons = (sbtn*)malloc(sizeof(sbtn) * the_game_opts.wx * the_game_opts.wy);
        for (int y = 0; y < the_game_opts.wy; y++) {
            for (int x = 0; x < the_game_opts.wx; x++) {
                board_buttons[y * the_game_opts.wx + x] = sbtn{
                    static_cast<float>(x)*(padding),
                    static_cast<float>(y)*(padding),
                    button_size, false, false
                };
            }
        }
    }

    TwixT_PP::~TwixT_PP()
    {}

    void TwixT_PP::set_game(game* new_game)
    {
        the_game = new_game;
        if (the_game) {
            twixt_pp_options* opts_ref;
            the_game->methods->get_options_bin_ref(the_game, (void**)&opts_ref);
            the_game_opts = *opts_ref;
            the_game_int = (twixt_pp_internal_methods*)the_game->methods->internal_methods;
            board_buttons = (sbtn*)malloc(sizeof(sbtn) * the_game_opts.wx * the_game_opts.wy);
        } else {
            the_game_opts = (twixt_pp_options){
                .wx = 24,
                .wy = 24,
            };
            the_game_int = NULL;
            free(board_buttons);
            board_buttons = (sbtn*)malloc(sizeof(sbtn) * the_game_opts.wx * the_game_opts.wy);
            for (int y = 0; y < the_game_opts.wy; y++) {
                for (int x = 0; x < the_game_opts.wx; x++) {
                    board_buttons[y * the_game_opts.wx + x] = sbtn{
                        static_cast<float>(x)*(padding),
                        static_cast<float>(y)*(padding),
                        button_size, false, false
                    };
                }
            }
        }
    }

    void TwixT_PP::process_event(SDL_Event event)
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
                    int mX = event.button.x - x_px;
                    int mY = event.button.y - y_px;
                    mX -= w_px/2-(padding*the_game_opts.wx)/2 + padding/2;
                    mY -= h_px/2-(padding*the_game_opts.wy)/2 + padding/2;
                    for (int y = 0; y < the_game_opts.wy; y++) {
                        for (int x = 0; x < the_game_opts.wx; x++) {
                            board_buttons[y * the_game_opts.wx + x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                TWIXT_PP_PLAYER node_player;
                                the_game_int->get_node(the_game, x, y, &node_player);
                                if (board_buttons[y * the_game_opts.wx + x].hovered && board_buttons[y * the_game_opts.wx + x].mousedown && node_player == TWIXT_PP_PLAYER_NONE) {
                                    uint64_t move_code = (x << 8) | y;
                                    Control::main_client->inbox.push(Control::f_event_game_move(move_code));
                                }
                                board_buttons[y * the_game_opts.wx + x].mousedown = false;
                            }
                            board_buttons[y * the_game_opts.wx + x].mousedown |= (board_buttons[y * the_game_opts.wx + x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        }
                    }
                }
            } break;
        }
    }

    void TwixT_PP::update()
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(padding*the_game_opts.wx)/2 + padding/2;
        mY -= h_px/2-(padding*the_game_opts.wy)/2 + padding/2;
        for (int y = 0; y < the_game_opts.wy; y++) {
            for (int x = 0; x < the_game_opts.wx; x++) {
                board_buttons[y * the_game_opts.wx + x].x  = static_cast<float>(x)*(padding);
                board_buttons[y * the_game_opts.wx + x].y  = static_cast<float>(y)*(padding);
                board_buttons[y * the_game_opts.wx + x].r = button_size;
                board_buttons[y * the_game_opts.wx + x].update(mX, mY);
                if ((x == 0 && y == 0) || (x == the_game_opts.wx - 1 && y == 0) || (x == 0 && y == the_game_opts.wy - 1) || (x == the_game_opts.wx - 1 && y == the_game_opts.wy - 1)) {
                    board_buttons[y * the_game_opts.wx + x].hovered = false;
                }
                if (((x == 0 || x == the_game_opts.wx - 1) && pbuf == TWIXT_PP_PLAYER_WHITE) || (y == 0 || y == the_game_opts.wy - 1) && pbuf == TWIXT_PP_PLAYER_BLACK) {
                    board_buttons[y * the_game_opts.wx + x].hovered = false;
                }
            }
        }
    }

    void TwixT_PP::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2-(padding*the_game_opts.wx)/2 + padding/2, h_px/2-(padding*the_game_opts.wy)/2 + padding/2);
        //TODO draw player boarder for current player

        // draw player backline background color
        nvgSave(dc);
        for (int i = 0; i < 2; i++) {
            nvgBeginPath(dc);
            nvgMoveTo(dc, -padding/2, -padding/2);
            nvgLineTo(dc, padding * the_game_opts.wx - padding/2, -padding/2);
            nvgLineTo(dc, padding * (the_game_opts.wx - 1) - padding/2, padding/2);
            nvgLineTo(dc, padding/2, padding/2);
            nvgFillColor(dc, nvgRGB(240, 217, 181)); // wood light
            nvgFill(dc);
            nvgBeginPath(dc);
            nvgMoveTo(dc, -padding/2, -padding/2);
            nvgLineTo(dc, -padding/2, padding * the_game_opts.wx - padding/2);
            nvgLineTo(dc, padding/2, padding * (the_game_opts.wx - 1) - padding/2);
            nvgLineTo(dc, padding/2, padding/2);
            nvgFillColor(dc, nvgRGB(161, 119, 67)); // wood dark
            nvgFill(dc);
            nvgTranslate(dc, padding * (the_game_opts.wx - 1), padding * (the_game_opts.wy - 1));
            nvgRotate(dc, M_PI);
        }
        nvgRestore(dc);

        for (int y = 0; y < the_game_opts.wy; y++) {
            for (int x = 0; x < the_game_opts.wx; x++) {
                if (!the_game) {
                    continue;
                }
                float base_x = static_cast<float>(x)*(padding);
                float base_y = static_cast<float>(y)*(padding);
                //TODO if drawing lines to be added on hover, display here
                uint8_t connections;
                the_game_int->get_node_connections(the_game, x, y, &connections);
                if (connections & TWIXT_PP_DIR_RT) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + 2 * padding, base_y - padding);
                    nvgStrokeWidth(dc, button_size*0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_RB) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + 2 * padding, base_y + padding);
                    nvgStrokeWidth(dc, button_size*0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_BR) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + padding, base_y + 2 * padding);
                    nvgStrokeWidth(dc, button_size*0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_BL) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x - padding, base_y + 2 * padding);
                    nvgStrokeWidth(dc, button_size*0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
            }
        }
        for (int y = 0; y < the_game_opts.wy; y++) {
            for (int x = 0; x < the_game_opts.wx; x++) {
                float base_x = static_cast<float>(x)*(padding);
                float base_y = static_cast<float>(y)*(padding);
                TWIXT_PP_PLAYER np = TWIXT_PP_PLAYER_NONE;
                if (the_game) {
                    the_game_int->get_node(the_game, x, y, &np);
                } else {
                    if ((x == 0 && y == 0) || (x == the_game_opts.wx - 1 && y == 0) || (x == 0 && y == the_game_opts.wy - 1) || (x == the_game_opts.wx - 1 && y == the_game_opts.wy - 1)) {
                        np = TWIXT_PP_PLAYER_INVALID;
                    }
                }
                switch (np) {
                    case TWIXT_PP_PLAYER_NONE: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, button_size*0.2);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        nvgFill(dc);
                    } break;
                    case TWIXT_PP_PLAYER_WHITE: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                        nvgFillColor(dc, nvgRGB(236, 236, 236));
                        nvgFill(dc);
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                        nvgStrokeWidth(dc, button_size*0.2);
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        nvgStroke(dc);
                    } break;
                    case TWIXT_PP_PLAYER_BLACK: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, button_size);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        nvgFill(dc);
                    } break;
                    default: break;
                }
                if (!the_game) {
                    continue;
                }
                if (board_buttons[y * the_game_opts.wx + x].hovered == true && np == TWIXT_PP_PLAYER_NONE) {
                    nvgBeginPath(dc);
                    nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                    nvgStrokeWidth(dc, button_size*0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);

                    nvgBeginPath(dc);
                    nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                    nvgStrokeWidth(dc, button_size*0.15); // TODO maybe this thinner to increase contrast for white backline hovers
                    if (pbuf == TWIXT_PP_PLAYER_WHITE) {
                        nvgStrokeColor(dc, nvgRGB(236, 236, 236));
                    } else {
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    }
                    nvgStroke(dc);
                }
            }
        }
        nvgRestore(dc);
    }

    void TwixT_PP::draw_options()
    {
        ImGui::SliderFloat("padding", &padding, 10, 80, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        button_size = 0.45 * padding;
        // ImGui::SliderFloat("button size", &button_size, 3, padding * 0.45, "%.3f", ImGuiSliderFlags_AlwaysClamp); //TODO unlock button size
        //TODO display collision free lines when hovering
    }

    TwixT_PP_FEW::TwixT_PP_FEW():
        FrontendWrap("TwixT_PP")
    {}

    TwixT_PP_FEW::~TwixT_PP_FEW()
    {}
    
    bool TwixT_PP_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::TwixT_PP*>(base_game_variant) != nullptr);
    }
    
    Frontend* TwixT_PP_FEW::new_frontend()
    {
        return new TwixT_PP();
    }

    void TwixT_PP_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
