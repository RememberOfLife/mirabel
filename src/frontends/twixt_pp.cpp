#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event_queue.h"
#include "control/event.h"
#include "games/game_catalogue.hpp"
#include "games/twixt_pp.hpp"

#include "frontends/twixt_pp.hpp"

namespace Frontends {

    void TwixT_PP::sbtn::update(float mx, float my) {
        hovered = (hypot(x - mx, y - my) < r);
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
                if (display_rankfile) {
                    my -= padding * 0.25;
                }
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - x_px;
                    int mY = event.button.y - y_px;
                    if (display_rankfile) {
                        mY -= padding * 0.25;
                    }
                    mX -= w_px/2-(padding*the_game_opts.wx)/2 + padding/2;
                    mY -= h_px/2-(padding*the_game_opts.wy)/2 + padding/2;
                    // detect swap button press
                    int mXp = mX + padding/2;
                    int mYp = mY + padding/2;
                    // MetaGui::logf("%d %d\n", mXp, mYp);
                    if (mXp >= 0 && mYp >= 0 && mXp <= padding && mYp <= padding) {
                        the_game_int->can_swap(the_game, &swap_hover);
                        if (swap_hover && swap_down && event.type == SDL_MOUSEBUTTONUP) {
                            f_event_any es;
                            f_event_create_game_move(&es, TWIXT_PP_MOVE_SWAP);
                            f_event_queue_push(&Control::main_client->inbox, &es);
                            swap_down = false;
                        }
                        swap_down = (event.type == SDL_MOUSEBUTTONDOWN);
                    }
                    for (int y = 0; y < the_game_opts.wy; y++) {
                        for (int x = 0; x < the_game_opts.wx; x++) {
                            board_buttons[y * the_game_opts.wx + x].update(mX, mY);
                            if ((x == 0 && y == 0) || (x == the_game_opts.wx - 1 && y == 0) || (x == 0 && y == the_game_opts.wy - 1) || (x == the_game_opts.wx - 1 && y == the_game_opts.wy - 1)) {
                                board_buttons[y * the_game_opts.wx + x].hovered = false;
                            }
                            if (((x == 0 || x == the_game_opts.wx - 1) && pbuf == TWIXT_PP_PLAYER_WHITE) || (y == 0 || y == the_game_opts.wy - 1) && pbuf == TWIXT_PP_PLAYER_BLACK) {
                                board_buttons[y * the_game_opts.wx + x].hovered = false;
                            }
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                TWIXT_PP_PLAYER node_player;
                                the_game_int->get_node(the_game, x, y, &node_player);
                                if (board_buttons[y * the_game_opts.wx + x].hovered && board_buttons[y * the_game_opts.wx + x].mousedown && node_player == TWIXT_PP_PLAYER_NONE) {
                                    uint64_t move_code = (x << 8) | y;
                                    f_event_any es;
                                    f_event_create_game_move(&es, move_code);
                                    f_event_queue_push(&Control::main_client->inbox, &es);
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
        if (auto_size) {
            float h_pad = h_px / (the_game_opts.wy + 1);
            float w_pad = w_px / (the_game_opts.wx + 2);
            rankfile_yoffset = h_pad < w_pad;
            padding = (h_pad < w_pad ? h_pad : w_pad);
            button_size = 0.45 * padding;
        }

        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(padding*the_game_opts.wx)/2 + padding/2;
        mY -= h_px/2-(padding*the_game_opts.wy)/2 + padding/2;

        hover_rank = -1;
        hover_file = -1;
        // determine hover rank and file, not button based, but range based
        //TODO fix some off by one pixel edge cases on the border of the board
        //TODO when game runs out rank file display freezes
        int mXp = mX + padding/2;
        int mYp = mY + padding/2;
        if (mXp >= 0 && mYp >= 0 && mXp <= padding * the_game_opts.wx && mYp <= padding * the_game_opts.wy) {
            hover_rank = mYp / padding;
            hover_file = mXp / padding;
        }
        if (hover_file == 0 && hover_rank == 0) {
            the_game_int->can_swap(the_game, &swap_hover);
        } else {
            swap_hover = false;
        }
        if ((hover_file == 0 && hover_rank == 0) || (hover_file == the_game_opts.wx - 1 && hover_rank == 0) || (hover_file == 0 && hover_rank == the_game_opts.wy - 1) || (hover_file == the_game_opts.wx - 1 && hover_rank == the_game_opts.wy - 1)) {
            hover_rank = -1;
            hover_file = -1;
        }

        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
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
        nvgTranslate(dc, w_px/2-(padding*the_game_opts.wx)/2 + padding/2, h_px/2-(padding*the_game_opts.wy)/2 + padding*0.5);
        if (display_rankfile) {
            nvgTranslate(dc, 0, padding * 0.25);
            if (!rankfile_yoffset) {
                nvgTranslate(dc, padding * 0.25, 0);
            }
        }

        //TODO draw player border for current player, needed with hover color indicator?

        if (display_rankfile) {
            // display rank and file descriptions
            nvgSave(dc);
            nvgTranslate(dc, -padding, -padding);
            
            nvgFontSize(dc, padding * 0.5);
            nvgFontFace(dc, "ff");
            char char_buf[4];

            for (uint8_t ix = 0; ix < the_game_opts.wx; ix++) {
                uint8_t ixw = ix;
                if (ixw > 25) {
                    char_buf[0] = 'a' + (ixw / 26) - 1;
                    ixw = ixw - (26 * (ixw / 26));
                    char_buf[1] = 'a' + ixw;
                    char_buf[2] = '\0';
                } else {
                    char_buf[0] = 'a' + ixw;
                    char_buf[1] = '\0';
                }
                nvgBeginPath(dc);
                if (hover_file == ix) {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                } else {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                }
                nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                nvgText(dc, padding * (ix + 1), padding * 0.35, char_buf, NULL);
            }

            for (uint8_t iy = 0; iy < the_game_opts.wy; iy++) {
                sprintf(char_buf, "%hhu", (uint8_t)(iy + 1)); // clang does not like this without a cast?
                nvgBeginPath(dc);
                if (hover_rank == iy) {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                } else {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                }
                nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgText(dc, padding * 0.3, padding * (iy + 1), char_buf, NULL);
            }

            nvgRestore(dc);
        }

        if (display_analysis_background) {
            nvgBeginPath(dc);
            nvgRect(dc, 0, 0, padding * the_game_opts.wx - padding, padding * the_game_opts.wy - padding);
            nvgFillColor(dc, nvgRGB(230, 255, 204)); // littlegolem green
            nvgFill(dc);
        }

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
            nvgLineTo(dc, -padding/2, padding * the_game_opts.wy - padding/2);
            nvgLineTo(dc, padding/2, padding * (the_game_opts.wy - 1) - padding/2);
            nvgLineTo(dc, padding/2, padding/2);
            nvgFillColor(dc, nvgRGB(161, 119, 67)); // wood dark
            nvgFill(dc);
            nvgTranslate(dc, padding * (the_game_opts.wx - 1), padding * (the_game_opts.wy - 1));
            nvgRotate(dc, M_PI);
        }
        nvgRestore(dc);

        if (display_runoff_lines) {
            // draw centering and run-off lines
            nvgSave(dc);
            nvgTranslate(dc, padding, padding);
            NVGcolor rocol;
            if (display_analysis_background) {
                rocol = nvgRGB(211, 222, 200);
            } else {
                // rocol = nvgRGB(228, 166, 90); // lighter
                rocol = nvgRGB(176, 125, 62); // darker
            }
            // calculate run-off line for the DIR_RT in x major direction, then for height y major
            int owx = the_game_opts.wx - 2;
            int owy = the_game_opts.wy - 2;
            int swx = 0;
            int swy = 0;
            int shx = 0;
            int shy = 0;
            while (true) {
                swx += 2;
                swy += 1;
                if (swx + 2 >= owx || swy + 1 >= owy) {
                    break;
                }
            }
            while (true) {
                shx += 1;
                shy += 2;
                if (shx + 1 >= owx || shy + 2 >= owy) {
                    break;
                }
            }
            for (int i = 0; i < 4; i++) {
                int swx_w;
                int swy_w;
                int shx_w;
                int shy_w;
                if (i % 2 == 0) {
                    swx_w = swx;
                    swy_w = swy;
                    shx_w = shx;
                    shy_w = shy;
                    draw_dashed_line(
                        (padding*(owx))/2 - padding/2, (padding*(owy))/2 - padding * 2.5,
                        (padding*(owx))/2 - padding/2, (padding*(owy))/2 - padding/2,
                        button_size*0.25, button_size, rocol);
                } else {
                    swx_w = shy;
                    swy_w = shx;
                    shx_w = swy;
                    shy_w = swx;
                    draw_dashed_line(
                        (padding*(owy))/2 - padding/2, (padding*(owx))/2 - padding * 2.5,
                        (padding*(owy))/2 - padding/2, (padding*(owx))/2 - padding/2,
                        button_size*0.25, button_size, rocol);
                }
                //TODO line width little bit too big on smaller board sizes
                draw_dashed_line(
                    0, 0,
                    padding * swx_w, padding * swy_w,
                    button_size*0.25, button_size, rocol);
                draw_dashed_line(
                    0, 0,
                    padding * shx_w, padding * shy_w,
                    button_size*0.25, button_size, rocol);
                nvgTranslate(dc, padding * (i % 2 == 0 ? the_game_opts.wx - 3 : the_game_opts.wy - 3), 0);
                nvgRotate(dc, M_PI / 2);
            }
            nvgRestore(dc);
        }

        for (int y = 0; y < the_game_opts.wy; y++) {
            for (int x = 0; x < the_game_opts.wx; x++) {
                if (!the_game) {
                    continue;
                }
                float base_x = static_cast<float>(x)*(padding);
                float base_y = static_cast<float>(y)*(padding);
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
                TWIXT_PP_PLAYER np = TWIXT_PP_PLAYER_NONE;
                if (the_game) {
                    the_game_int->get_node(the_game, x, y, &np);
                } else {
                    if ((x == 0 && y == 0) || (x == the_game_opts.wx - 1 && y == 0) || (x == 0 && y == the_game_opts.wy - 1) || (x == the_game_opts.wx - 1 && y == the_game_opts.wy - 1)) {
                        np = TWIXT_PP_PLAYER_INVALID;
                    }
                }
                if (display_hover_connections && board_buttons[y * the_game_opts.wx + x].hovered == true && np == TWIXT_PP_PLAYER_NONE) {
                    // draw connections that would be created if the hover was placed
                    NVGcolor ccol = nvgRGBA(25, 25, 25, 100);
                    if (display_hover_connections && board_buttons[y * the_game_opts.wx + x].mousedown == true) {
                        ccol = nvgRGBA(25, 25, 25, 180);
                    }
                    draw_cond_connection(base_x, base_y, x, y, TWIXT_PP_DIR_RT, ccol);
                    draw_cond_connection(base_x, base_y, x, y, TWIXT_PP_DIR_RB, ccol);
                    draw_cond_connection(base_x, base_y, x, y, TWIXT_PP_DIR_BR, ccol);
                    draw_cond_connection(base_x, base_y, x, y, TWIXT_PP_DIR_BL, ccol);
                    draw_cond_connection(base_x - 2 * padding, base_y + padding, x - 2, y + 1, TWIXT_PP_DIR_RT, ccol);
                    draw_cond_connection(base_x - 2 * padding, base_y - padding, x - 2, y - 1, TWIXT_PP_DIR_RB, ccol);
                    draw_cond_connection(base_x - padding, base_y - 2 * padding, x - 1, y - 2, TWIXT_PP_DIR_BR, ccol);
                    draw_cond_connection(base_x + padding, base_y - 2 * padding, x + 1, y - 2, TWIXT_PP_DIR_BL, ccol);
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
                    case TWIXT_PP_PLAYER_INVALID: {
                        bool can_swap = false;
                        if (the_game) {
                            the_game_int->can_swap(the_game, &can_swap);
                        }
                        if (can_swap) {
                            if (swap_hover) {
                                nvgBeginPath(dc);
                                nvgRect(dc, -padding*0.45, -padding*0.45, padding*0.9, padding*0.9);
                                if (swap_down) {
                                    nvgFillColor(dc, nvgRGBA(0, 0, 0, 30));
                                } else {
                                    nvgFillColor(dc, nvgRGBA(0, 0, 0, 15));
                                }
                                nvgFill(dc);
                            }
                            nvgFontSize(dc, padding * 0.5);
                            nvgFontFace(dc, "ff");
                            nvgFillColor(dc, nvgRGB(25, 25, 25));
                            nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                            nvgText(dc, 0, 0, "SW", NULL);
                            nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                            nvgText(dc, 0, 0, "AP", NULL);
                        }
                    } break;
                }
                if (!the_game) {
                    continue;
                }
                if (board_buttons[y * the_game_opts.wx + x].hovered == true && np == TWIXT_PP_PLAYER_NONE) {
                    if (board_buttons[y * the_game_opts.wx + x].mousedown) {
                        if (pbuf == TWIXT_PP_PLAYER_WHITE) {
                            nvgBeginPath(dc);
                            nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                            nvgFillColor(dc, nvgRGBA(236, 236, 236, 150));
                            nvgFill(dc);
                            nvgBeginPath(dc);
                        } else {
                            nvgBeginPath(dc);
                            nvgCircle(dc, base_x, base_y, button_size - button_size * 0.1);
                            nvgFillColor(dc, nvgRGBA(25, 25, 25, 150));
                            nvgFill(dc);
                            nvgBeginPath(dc);
                        }
                    }
                    if (display_hover_indicator_cross) {
                        nvgBeginPath(dc);
                        nvgStrokeWidth(dc, button_size*0.4);
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        nvgMoveTo(dc, base_x-button_size*0.75, base_y-button_size*0.75);
                        nvgLineTo(dc, base_x+button_size*0.75, base_y+button_size*0.75);
                        nvgMoveTo(dc, base_x-button_size*0.75, base_y+button_size*0.75);
                        nvgLineTo(dc, base_x+button_size*0.75, base_y-button_size*0.75);
                        nvgStroke(dc);

                        nvgBeginPath(dc);
                        nvgStrokeWidth(dc, button_size*0.3);
                        if (pbuf == TWIXT_PP_PLAYER_WHITE) {
                            nvgStrokeColor(dc, nvgRGB(236, 236, 236));
                        } else {
                            nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        }
                        nvgMoveTo(dc, base_x-button_size*0.7, base_y-button_size*0.7);
                        nvgLineTo(dc, base_x+button_size*0.7, base_y+button_size*0.7);
                        nvgMoveTo(dc, base_x-button_size*0.7, base_y+button_size*0.7);
                        nvgLineTo(dc, base_x+button_size*0.7, base_y-button_size*0.7);
                        nvgStroke(dc);
                    } else {
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
        }
        nvgRestore(dc);
    }

    void TwixT_PP::draw_options()
    {
        ImGui::Checkbox("auto-size", &auto_size);
        if (auto_size) {
            ImGui::BeginDisabled();
        }
        ImGui::SliderFloat("size", &padding, 10, 80, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        if (auto_size) {
            ImGui::EndDisabled();
        }
        ImGui::Checkbox("analysis background", &display_analysis_background);
        ImGui::Checkbox("hover style: cross", &display_hover_indicator_cross);
        ImGui::Checkbox("hover connections", &display_hover_connections);
        ImGui::Checkbox("run-off lines", &display_runoff_lines);
        ImGui::Checkbox("rank & file", &display_rankfile);
    }

    // draw dashed line with width w, color col and gap size g, will never under/over-draw
    // colorized at both ends
    //TODO different lengths of dashes vs gaps
    void TwixT_PP::draw_dashed_line(float x1, float y1, float x2, float y2, float w, float g, NVGcolor col)
    {
        nvgSave(dc);

        nvgTranslate(dc, x1, y1);
        x2 -= x1;
        y2 -= y1;

        nvgRotate(dc, atan2(y2, x2));
        float d = hypot(x2, y2);

        nvgStrokeColor(dc, col);
        nvgStrokeWidth(dc, w);
        bool dash_gap = false;
        float c = g;
        for (float t = 0; t < d; t += g) {
            if (t + g >= d) {
                dash_gap = false;
                c = d - t;
            }
            if (!dash_gap) {
                nvgBeginPath(dc);

                nvgMoveTo(dc, t, 0);
                nvgLineTo(dc, t+c, 0);

                nvgStroke(dc);
            }
            dash_gap = !dash_gap;
        }
        nvgRestore(dc);
    }

    // tries to draw the connection line, if the point exists and collision is unset
    void TwixT_PP::draw_cond_connection(float bx, float by, uint8_t x, uint8_t y, TWIXT_PP_DIR d, NVGcolor ccol)
    {
        TWIXT_PP_PLAYER np;
        the_game_int->get_node(the_game, x, y, &np);
        TWIXT_PP_PLAYER tp;
        switch (d) {
            case TWIXT_PP_DIR_RT: {
                the_game_int->get_node(the_game, x + 2, y - 1, &tp);
            } break;
            case TWIXT_PP_DIR_RB: {
                the_game_int->get_node(the_game, x + 2, y + 1, &tp);
            } break;
            case TWIXT_PP_DIR_BR: {
                the_game_int->get_node(the_game, x + 1, y + 2, &tp);
            } break;
            case TWIXT_PP_DIR_BL: {
                the_game_int->get_node(the_game, x - 1, y + 2, &tp);
            } break;
        }
        // put the existing player into np
        if (tp != TWIXT_PP_PLAYER_NONE && tp != TWIXT_PP_PLAYER_INVALID) {
            np = tp;
        }
        if (np == TWIXT_PP_PLAYER_NONE || np == TWIXT_PP_PLAYER_INVALID || np != pbuf) {
            return;
        }
        uint8_t collisions;
        the_game_int->get_node_collisions(the_game, x, y, &collisions);
        if (np == TWIXT_PP_PLAYER_WHITE) {
            collisions >>= 4;
        }
        if (collisions & d) {
            return;
        }
        nvgBeginPath(dc);
        nvgMoveTo(dc, bx, by);
        switch (d) {
            case TWIXT_PP_DIR_RT: {
                nvgLineTo(dc, bx + 2 * padding, by - padding);
            } break;
            case TWIXT_PP_DIR_RB: {
                nvgLineTo(dc, bx + 2 * padding, by + padding);
            } break;
            case TWIXT_PP_DIR_BR: {
                nvgLineTo(dc, bx + padding, by + 2 * padding);
            } break;
            case TWIXT_PP_DIR_BL: {
                nvgLineTo(dc, bx - padding, by + 2 * padding);
            } break;
        }
        nvgStrokeWidth(dc, button_size*0.2);
        nvgStrokeColor(dc, ccol);
        nvgStroke(dc);
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
