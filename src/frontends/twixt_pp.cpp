#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    struct sbtn {
        float x;
        float y;
        float r;
        bool hovered;
        bool mousedown;

        void update(float mx, float my)
        {
            hovered = (hypot(x - mx, y - my) < r);
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        game g = (game){
            .methods = NULL};
        const twixt_pp_internal_methods* gi = NULL;
        twixt_pp_options opts = (twixt_pp_options){
            .wx = 24,
            .wy = 24,
        };

        player_id ptm = PLAYER_NONE;
        player_id res = PLAYER_NONE;

        float padding = 38;
        float button_size = 10;
        bool auto_size = true;
        bool display_analysis_background = true;
        bool display_hover_indicator_cross = false;
        bool display_hover_connections = true;
        bool display_runoff_lines = true;
        bool display_rankfile = true;

        bool rankfile_yoffset = true;

        int mx = 0;
        int my = 0;

        sbtn* board_buttons; // board_buttons origin is top left, row major
        bool swap_hover = false;
        bool swap_down = false;

        int hover_rank;
        int hover_file;
    };

    data_repr& _get_repr(frontend* self)
    {
        return *((data_repr*)(self->data1));
    }

    // draw dashed line with width w, color col and gap size g, will never under/over-draw
    // colorized at both ends
    //TODO different lengths of dashes vs gaps
    void draw_dashed_line(frontend* self, float x1, float y1, float x2, float y2, float w, float g, NVGcolor col)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
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
                nvgLineTo(dc, t + c, 0);

                nvgStroke(dc);
            }
            dash_gap = !dash_gap;
        }
        nvgRestore(dc);
    }

    // tries to draw the connection line, if the point exists and collision is unset
    void draw_cond_connection(frontend* self, float bx, float by, uint8_t x, uint8_t y, TWIXT_PP_DIR d, NVGcolor ccol)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
        TWIXT_PP_PLAYER np;
        data.gi->get_node(&data.g, x, y, &np);
        TWIXT_PP_PLAYER tp;
        switch (d) {
            case TWIXT_PP_DIR_RT: {
                data.gi->get_node(&data.g, x + 2, y - 1, &tp);
            } break;
            case TWIXT_PP_DIR_RB: {
                data.gi->get_node(&data.g, x + 2, y + 1, &tp);
            } break;
            case TWIXT_PP_DIR_BR: {
                data.gi->get_node(&data.g, x + 1, y + 2, &tp);
            } break;
            case TWIXT_PP_DIR_BL: {
                data.gi->get_node(&data.g, x - 1, y + 2, &tp);
            } break;
        }
        // put the existing player into np
        if (tp != TWIXT_PP_PLAYER_NONE && tp != TWIXT_PP_PLAYER_INVALID) {
            np = tp;
        }
        if (np == TWIXT_PP_PLAYER_NONE || np == TWIXT_PP_PLAYER_INVALID || np != data.ptm) {
            return;
        }
        uint8_t collisions;
        data.gi->get_node_collisions(&data.g, x, y, &collisions);
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
                nvgLineTo(dc, bx + 2 * data.padding, by - data.padding);
            } break;
            case TWIXT_PP_DIR_RB: {
                nvgLineTo(dc, bx + 2 * data.padding, by + data.padding);
            } break;
            case TWIXT_PP_DIR_BR: {
                nvgLineTo(dc, bx + data.padding, by + 2 * data.padding);
            } break;
            case TWIXT_PP_DIR_BL: {
                nvgLineTo(dc, bx - data.padding, by + 2 * data.padding);
            } break;
        }
        nvgStrokeWidth(dc, data.button_size * 0.2);
        nvgStrokeColor(dc, ccol);
        nvgStroke(dc);
    }

    const char* get_last_error(frontend* self)
    {
        //TODO
        return NULL;
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        //TODO
        self->data1 = malloc(sizeof(data_repr));
        new (self->data1) data_repr;
        data_repr& data = _get_repr(self);
        data.dc = Control::main_client->nanovg_ctx;
        data.dd = display_data;
        data.board_buttons = (sbtn*)malloc(sizeof(sbtn) * data.opts.wx * data.opts.wy);
        memset((char*)data.board_buttons, 0x00, sizeof(sbtn) * data.opts.wx * data.opts.wy);
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        data_repr& data = _get_repr(self);
        free(data.board_buttons);
        delete (data_repr*)self->data1;
        self->data1 = NULL;
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        data_repr& data = _get_repr(self);
        ImGui::Checkbox("auto-size", &data.auto_size);
        if (data.auto_size) {
            ImGui::BeginDisabled();
        }
        ImGui::SliderFloat("size", &data.padding, 10, 80, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        if (data.auto_size) {
            ImGui::EndDisabled();
        }
        ImGui::Checkbox("analysis background", &data.display_analysis_background);
        ImGui::Checkbox("hover style: cross", &data.display_hover_indicator_cross);
        ImGui::Checkbox("hover connections", &data.display_hover_connections);
        ImGui::Checkbox("run-off lines", &data.display_runoff_lines);
        ImGui::Checkbox("rank & file", &data.display_rankfile);
        return ERR_OK;
    }

    error_code process_event(frontend* self, event_any event)
    {
        data_repr& data = _get_repr(self);
        bool dirty = false;
        switch (event.base.type) {
            case EVENT_TYPE_HEARTBEAT: {
                event_queue_push(data.dd->outbox, &event);
            } break;
            case EVENT_TYPE_GAME_LOAD_METHODS: {
                if (data.g.methods) {
                    game_destroy(&data.g);
                }
                data.g.methods = event.game_load_methods.methods;
                data.g.data1 = NULL;
                data.g.data2 = NULL;
                game_create(&data.g, &event.game_load_methods.init_info);
                data.gi = (const twixt_pp_internal_methods*)data.g.methods->internal_methods;
                //TODO could use internal method for this
                size_t size_fill;
                const char* opts_export;
                game_export_options(&data.g, PLAYER_NONE, &size_fill, &opts_export);
                {
                    // parse twixt opts: format y/x+ or s+
                    data.opts.wy = 0;
                    const char* wp = opts_export;
                    while (*wp != '\0' && *wp != '/' && *wp != '+') {
                        data.opts.wy *= 10;
                        data.opts.wy += (*wp - '0');
                        wp++;
                    }
                    if (*wp == '/') {
                        wp++;
                        data.opts.wx = 0;
                        while (*wp != '\0' && *wp != '+') {
                            data.opts.wx *= 10;
                            data.opts.wx += (*wp - '0');
                            wp++;
                        }
                    } else {
                        data.opts.wx = data.opts.wy;
                    }
                    data.opts.pie_swap = (*wp == '+');
                }
                free(data.board_buttons);
                data.board_buttons = (sbtn*)malloc(sizeof(sbtn) * data.opts.wx * data.opts.wy);
                memset((char*)data.board_buttons, 0x00, sizeof(sbtn) * data.opts.wx * data.opts.wy);
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    game_destroy(&data.g);
                }
                data.g.methods = NULL;
                data.gi = NULL;
                data.opts = (twixt_pp_options){
                    .wx = 24,
                    .wy = 24,
                };
                free(data.board_buttons);
                data.board_buttons = (sbtn*)malloc(sizeof(sbtn) * data.opts.wx * data.opts.wy);
                memset((char*)data.board_buttons, 0x00, sizeof(sbtn) * data.opts.wx * data.opts.wy);
                dirty = false;
            } break;
            case EVENT_TYPE_GAME_STATE: {
                game_import_state(&data.g, event.game_state.state);
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                game_make_move(&data.g, event.game_move.player, event.game_move.data);
                dirty = true;
            } break;
            default: {
                // pass
            } break;
        }
        event_destroy(&event);
        if (dirty) {
            data.ptm = PLAYER_NONE;
            data.res = PLAYER_NONE;
            uint8_t pbuf_c;
            const player_id* pbuf;
            game_players_to_move(&data.g, &pbuf_c, &pbuf);
            if (pbuf_c == 0) {
                game_get_results(&data.g, &pbuf_c, &pbuf);
                if (pbuf_c > 0) {
                    data.res = pbuf[0];
                }
            } else {
                data.ptm = pbuf[0];
            }
        }
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        data_repr& data = _get_repr(self);
        //BUG this can perform a click after previous window resizing in the same loop, while operating on the not yet updated button positions/sizes
        if (data.g.methods == NULL || data.ptm == PLAYER_NONE) {
            return ERR_OK;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                data.mx = event.motion.x - data.dd->x;
                data.my = event.motion.y - data.dd->y;
                if (data.display_rankfile) {
                    data.my -= data.padding * 0.25;
                }
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - data.dd->x;
                    int mY = event.button.y - data.dd->y;
                    if (data.display_rankfile) {
                        mY -= data.padding * 0.25;
                    }
                    mX -= data.dd->w / 2 - (data.padding * data.opts.wx) / 2 + data.padding / 2;
                    mY -= data.dd->h / 2 - (data.padding * data.opts.wy) / 2 + data.padding / 2;
                    // detect swap button press
                    int mXp = mX + data.padding / 2;
                    int mYp = mY + data.padding / 2;
                    if (mXp >= 0 && mYp >= 0 && mXp <= data.padding && mYp <= data.padding) {
                        data.gi->can_swap(&data.g, &data.swap_hover);
                        if (data.swap_hover && data.swap_down && event.type == SDL_MOUSEBUTTONUP) {
                            event_any es;
                            event_create_game_move(&es, data.ptm, game_e_create_move_sync_small(&data.g, TWIXT_PP_MOVE_SWAP));
                            event_queue_push(data.dd->outbox, &es);
                            data.swap_down = false;
                        }
                        data.swap_down = (event.type == SDL_MOUSEBUTTONDOWN);
                    }
                    for (int y = 0; y < data.opts.wy; y++) {
                        for (int x = 0; x < data.opts.wx; x++) {
                            data.board_buttons[y * data.opts.wx + x].update(mX, mY);
                            if ((x == 0 && y == 0) || (x == data.opts.wx - 1 && y == 0) || (x == 0 && y == data.opts.wy - 1) || (x == data.opts.wx - 1 && y == data.opts.wy - 1)) {
                                data.board_buttons[y * data.opts.wx + x].hovered = false;
                            }
                            if (((x == 0 || x == data.opts.wx - 1) && data.ptm == TWIXT_PP_PLAYER_WHITE) || (y == 0 || y == data.opts.wy - 1) && data.ptm == TWIXT_PP_PLAYER_BLACK) {
                                data.board_buttons[y * data.opts.wx + x].hovered = false;
                            }
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                TWIXT_PP_PLAYER node_player;
                                data.gi->get_node(&data.g, x, y, &node_player);
                                if (data.board_buttons[y * data.opts.wx + x].hovered && data.board_buttons[y * data.opts.wx + x].mousedown && node_player == TWIXT_PP_PLAYER_NONE) {
                                    uint64_t move_code = (x << 8) | y;
                                    event_any es;
                                    event_create_game_move(&es, data.ptm, game_e_create_move_sync_small(&data.g, move_code));
                                    event_queue_push(data.dd->outbox, &es);
                                }
                                data.board_buttons[y * data.opts.wx + x].mousedown = false;
                            }
                            data.board_buttons[y * data.opts.wx + x].mousedown |= (data.board_buttons[y * data.opts.wx + x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        }
                    }
                }
            } break;
        }
        return ERR_OK;
    }

    error_code update(frontend* self)
    {
        data_repr& data = _get_repr(self);

        if (data.auto_size) {
            float h_pad = data.dd->h / (data.opts.wy + 1);
            float w_pad = data.dd->w / (data.opts.wx + 2);
            data.rankfile_yoffset = h_pad < w_pad;
            data.padding = (h_pad < w_pad ? h_pad : w_pad);
            data.button_size = 0.45 * data.padding;
        }

        // set button hovered
        int mX = data.mx;
        int mY = data.my;
        mX -= data.dd->w / 2 - (data.padding * data.opts.wx) / 2 + data.padding / 2;
        mY -= data.dd->h / 2 - (data.padding * data.opts.wy) / 2 + data.padding / 2;

        data.hover_rank = -1;
        data.hover_file = -1;
        // determine hover rank and file, not button based, but range based
        //TODO fix some off by one pixel edge cases on the border of the board
        //TODO when game runs out rank file display freezes
        int mXp = mX + data.padding / 2;
        int mYp = mY + data.padding / 2;
        if (mXp >= 0 && mYp >= 0 && mXp <= data.padding * data.opts.wx && mYp <= data.padding * data.opts.wy) {
            data.hover_rank = mYp / data.padding;
            data.hover_file = mXp / data.padding;
        }
        if (data.hover_file == 0 && data.hover_rank == 0) {
            data.gi->can_swap(&data.g, &data.swap_hover);
        } else {
            data.swap_hover = false;
        }
        if ((data.hover_file == 0 && data.hover_rank == 0) || (data.hover_file == data.opts.wx - 1 && data.hover_rank == 0) || (data.hover_file == 0 && data.hover_rank == data.opts.wy - 1) || (data.hover_file == data.opts.wx - 1 && data.hover_rank == data.opts.wy - 1)) {
            data.hover_rank = -1;
            data.hover_file = -1;
        }

        //TODO put button pos/size recalc into sdl resize event
        //TODO when reloading the game after a game is done, the hover does not reset
        if (data.g.methods == NULL || data.ptm == PLAYER_NONE) {
            return ERR_OK;
        }
        for (int y = 0; y < data.opts.wy; y++) {
            for (int x = 0; x < data.opts.wx; x++) {
                data.board_buttons[y * data.opts.wx + x].x = static_cast<float>(x) * (data.padding);
                data.board_buttons[y * data.opts.wx + x].y = static_cast<float>(y) * (data.padding);
                data.board_buttons[y * data.opts.wx + x].r = data.button_size;
                data.board_buttons[y * data.opts.wx + x].update(mX, mY);
                //BUG why is it not a potential problem that the button mousedown is never initialized?
                if ((x == 0 && y == 0) || (x == data.opts.wx - 1 && y == 0) || (x == 0 && y == data.opts.wy - 1) || (x == data.opts.wx - 1 && y == data.opts.wy - 1)) {
                    data.board_buttons[y * data.opts.wx + x].hovered = false;
                }
                if (((x == 0 || x == data.opts.wx - 1) && data.ptm == TWIXT_PP_PLAYER_WHITE) || (y == 0 || y == data.opts.wy - 1) && data.ptm == TWIXT_PP_PLAYER_BLACK) {
                    data.board_buttons[y * data.opts.wx + x].hovered = false;
                }
            }
        }

        return ERR_OK;
    }

    error_code render(frontend* self)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
        frontend_display_data& dd = *data.dd;

        nvgBeginFrame(dc, dd.fbw, dd.fbh, 2); //TODO use proper devicePixelRatio

        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, dd.w + 20, dd.h + 20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, dd.w / 2 - (data.padding * data.opts.wx) / 2 + data.padding / 2, dd.h / 2 - (data.padding * data.opts.wy) / 2 + data.padding * 0.5);
        if (data.display_rankfile) {
            nvgTranslate(dc, 0, data.padding * 0.25);
            if (!data.rankfile_yoffset) {
                nvgTranslate(dc, data.padding * 0.25, 0);
            }
        }

        //TODO draw player border for current player, needed with hover color indicator?

        if (data.display_rankfile) {
            // display rank and file descriptions
            nvgSave(dc);
            nvgTranslate(dc, -data.padding, -data.padding);

            nvgFontSize(dc, data.padding * 0.5);
            nvgFontFace(dc, "ff");
            char char_buf[4];

            for (uint8_t ix = 0; ix < data.opts.wx; ix++) {
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
                if (data.hover_file == ix) {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                } else {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                }
                nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                nvgText(dc, data.padding * (ix + 1), data.padding * 0.35, char_buf, NULL);
            }

            for (uint8_t iy = 0; iy < data.opts.wy; iy++) {
                sprintf(char_buf, "%hhu", (uint8_t)(iy + 1)); // clang does not like this without a cast?
                nvgBeginPath(dc);
                if (data.hover_rank == iy) {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                } else {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                }
                nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgText(dc, data.padding * 0.3, data.padding * (iy + 1), char_buf, NULL);
            }

            nvgRestore(dc);
        }

        if (data.display_analysis_background) {
            nvgBeginPath(dc);
            nvgRect(dc, 0, 0, data.padding * data.opts.wx - data.padding, data.padding * data.opts.wy - data.padding);
            nvgFillColor(dc, nvgRGB(230, 255, 204)); // littlegolem green
            nvgFill(dc);
        }

        // draw player backline background color
        nvgSave(dc);
        for (int i = 0; i < 2; i++) {
            nvgBeginPath(dc);
            nvgMoveTo(dc, -data.padding / 2, -data.padding / 2);
            nvgLineTo(dc, data.padding * data.opts.wx - data.padding / 2, -data.padding / 2);
            nvgLineTo(dc, data.padding * (data.opts.wx - 1) - data.padding / 2, data.padding / 2);
            nvgLineTo(dc, data.padding / 2, data.padding / 2);
            nvgFillColor(dc, nvgRGB(240, 217, 181)); // wood light
            nvgFill(dc);
            nvgBeginPath(dc);
            nvgMoveTo(dc, -data.padding / 2, -data.padding / 2);
            nvgLineTo(dc, -data.padding / 2, data.padding * data.opts.wy - data.padding / 2);
            nvgLineTo(dc, data.padding / 2, data.padding * (data.opts.wy - 1) - data.padding / 2);
            nvgLineTo(dc, data.padding / 2, data.padding / 2);
            nvgFillColor(dc, nvgRGB(161, 119, 67)); // wood dark
            nvgFill(dc);
            nvgTranslate(dc, data.padding * (data.opts.wx - 1), data.padding * (data.opts.wy - 1));
            nvgRotate(dc, M_PI);
        }
        nvgRestore(dc);

        if (data.display_runoff_lines) {
            // draw centering and run-off lines
            nvgSave(dc);
            nvgTranslate(dc, data.padding, data.padding);
            NVGcolor rocol;
            if (data.display_analysis_background) {
                rocol = nvgRGB(211, 222, 200);
            } else {
                // rocol = nvgRGB(228, 166, 90); // lighter
                rocol = nvgRGB(176, 125, 62); // darker
            }
            // calculate run-off line for the DIR_RT in x major direction, then for height y major
            int owx = data.opts.wx - 2;
            int owy = data.opts.wy - 2;
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
                    draw_dashed_line(self, (data.padding * (owx)) / 2 - data.padding / 2, (data.padding * (owy)) / 2 - data.padding * 2.5, (data.padding * (owx)) / 2 - data.padding / 2, (data.padding * (owy)) / 2 - data.padding / 2, data.button_size * 0.25, data.button_size, rocol);
                } else {
                    swx_w = shy;
                    swy_w = shx;
                    shx_w = swy;
                    shy_w = swx;
                    draw_dashed_line(self, (data.padding * (owy)) / 2 - data.padding / 2, (data.padding * (owx)) / 2 - data.padding * 2.5, (data.padding * (owy)) / 2 - data.padding / 2, (data.padding * (owx)) / 2 - data.padding / 2, data.button_size * 0.25, data.button_size, rocol);
                }
                //TODO line width little bit too big on smaller board sizes
                draw_dashed_line(
                    self,
                    0,
                    0,
                    data.padding * swx_w,
                    data.padding * swy_w,
                    data.button_size * 0.25,
                    data.button_size,
                    rocol
                );
                draw_dashed_line(
                    self,
                    0,
                    0,
                    data.padding * shx_w,
                    data.padding * shy_w,
                    data.button_size * 0.25,
                    data.button_size,
                    rocol
                );
                nvgTranslate(dc, data.padding * (i % 2 == 0 ? data.opts.wx - 3 : data.opts.wy - 3), 0);
                nvgRotate(dc, M_PI / 2);
            }
            nvgRestore(dc);
        }

        for (int y = 0; y < data.opts.wy; y++) {
            for (int x = 0; x < data.opts.wx; x++) {
                if (data.g.methods == NULL) {
                    continue;
                }
                float base_x = static_cast<float>(x) * (data.padding);
                float base_y = static_cast<float>(y) * (data.padding);
                uint8_t connections;
                data.gi->get_node_connections(&data.g, x, y, &connections);
                if (connections & TWIXT_PP_DIR_RT) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + 2 * data.padding, base_y - data.padding);
                    nvgStrokeWidth(dc, data.button_size * 0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_RB) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + 2 * data.padding, base_y + data.padding);
                    nvgStrokeWidth(dc, data.button_size * 0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_BR) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + data.padding, base_y + 2 * data.padding);
                    nvgStrokeWidth(dc, data.button_size * 0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                if (connections & TWIXT_PP_DIR_BL) {
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x - data.padding, base_y + 2 * data.padding);
                    nvgStrokeWidth(dc, data.button_size * 0.2);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgStroke(dc);
                }
                TWIXT_PP_PLAYER np = TWIXT_PP_PLAYER_NONE;
                if (data.g.methods != NULL) {
                    data.gi->get_node(&data.g, x, y, &np);
                } else {
                    if ((x == 0 && y == 0) || (x == data.opts.wx - 1 && y == 0) || (x == 0 && y == data.opts.wy - 1) || (x == data.opts.wx - 1 && y == data.opts.wy - 1)) {
                        np = TWIXT_PP_PLAYER_INVALID;
                    }
                }
                if (data.display_hover_connections && data.board_buttons[y * data.opts.wx + x].hovered == true && np == TWIXT_PP_PLAYER_NONE) {
                    // draw connections that would be created if the hover was placed
                    NVGcolor ccol = nvgRGBA(25, 25, 25, 100);
                    if (data.display_hover_connections && data.board_buttons[y * data.opts.wx + x].mousedown == true) {
                        ccol = nvgRGBA(25, 25, 25, 180);
                    }
                    draw_cond_connection(self, base_x, base_y, x, y, TWIXT_PP_DIR_RT, ccol);
                    draw_cond_connection(self, base_x, base_y, x, y, TWIXT_PP_DIR_RB, ccol);
                    draw_cond_connection(self, base_x, base_y, x, y, TWIXT_PP_DIR_BR, ccol);
                    draw_cond_connection(self, base_x, base_y, x, y, TWIXT_PP_DIR_BL, ccol);
                    draw_cond_connection(self, base_x - 2 * data.padding, base_y + data.padding, x - 2, y + 1, TWIXT_PP_DIR_RT, ccol);
                    draw_cond_connection(self, base_x - 2 * data.padding, base_y - data.padding, x - 2, y - 1, TWIXT_PP_DIR_RB, ccol);
                    draw_cond_connection(self, base_x - data.padding, base_y - 2 * data.padding, x - 1, y - 2, TWIXT_PP_DIR_BR, ccol);
                    draw_cond_connection(self, base_x + data.padding, base_y - 2 * data.padding, x + 1, y - 2, TWIXT_PP_DIR_BL, ccol);
                }
            }
        }
        for (int y = 0; y < data.opts.wy; y++) {
            for (int x = 0; x < data.opts.wx; x++) {
                float base_x = static_cast<float>(x) * (data.padding);
                float base_y = static_cast<float>(y) * (data.padding);
                TWIXT_PP_PLAYER np = TWIXT_PP_PLAYER_NONE;
                if (data.g.methods != NULL) {
                    data.gi->get_node(&data.g, x, y, &np);
                } else {
                    if ((x == 0 && y == 0) || (x == data.opts.wx - 1 && y == 0) || (x == 0 && y == data.opts.wy - 1) || (x == data.opts.wx - 1 && y == data.opts.wy - 1)) {
                        np = TWIXT_PP_PLAYER_INVALID;
                    }
                }
                switch (np) {
                    case TWIXT_PP_PLAYER_NONE: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size * 0.2);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        nvgFill(dc);
                    } break;
                    case TWIXT_PP_PLAYER_WHITE: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                        nvgFillColor(dc, nvgRGB(236, 236, 236));
                        nvgFill(dc);
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                        nvgStrokeWidth(dc, data.button_size * 0.2);
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        nvgStroke(dc);
                    } break;
                    case TWIXT_PP_PLAYER_BLACK: {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        nvgFill(dc);
                    } break;
                    case TWIXT_PP_PLAYER_INVALID: {
                        bool can_swap = false;
                        if (data.g.methods != NULL) {
                            data.gi->can_swap(&data.g, &can_swap);
                        }
                        if (can_swap) {
                            if (data.swap_hover) {
                                nvgBeginPath(dc);
                                nvgRect(dc, -data.padding * 0.45, -data.padding * 0.45, data.padding * 0.9, data.padding * 0.9);
                                if (data.swap_down) {
                                    nvgFillColor(dc, nvgRGBA(0, 0, 0, 30));
                                } else {
                                    nvgFillColor(dc, nvgRGBA(0, 0, 0, 15));
                                }
                                nvgFill(dc);
                            }
                            nvgFontSize(dc, data.padding * 0.5);
                            nvgFontFace(dc, "ff");
                            nvgFillColor(dc, nvgRGB(25, 25, 25));
                            nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                            nvgText(dc, 0, 0, "SW", NULL);
                            nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                            nvgText(dc, 0, 0, "AP", NULL);
                        }
                    } break;
                }
                if (data.g.methods == NULL) {
                    continue;
                }
                if (data.board_buttons[y * data.opts.wx + x].hovered == true && np == TWIXT_PP_PLAYER_NONE) {
                    if (data.board_buttons[y * data.opts.wx + x].mousedown) {
                        if (data.ptm == TWIXT_PP_PLAYER_WHITE) {
                            nvgBeginPath(dc);
                            nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                            nvgFillColor(dc, nvgRGBA(236, 236, 236, 150));
                            nvgFill(dc);
                            nvgBeginPath(dc);
                        } else {
                            nvgBeginPath(dc);
                            nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                            nvgFillColor(dc, nvgRGBA(25, 25, 25, 150));
                            nvgFill(dc);
                            nvgBeginPath(dc);
                        }
                    }
                    if (data.display_hover_indicator_cross) {
                        nvgBeginPath(dc);
                        nvgStrokeWidth(dc, data.button_size * 0.4);
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        nvgMoveTo(dc, base_x - data.button_size * 0.75, base_y - data.button_size * 0.75);
                        nvgLineTo(dc, base_x + data.button_size * 0.75, base_y + data.button_size * 0.75);
                        nvgMoveTo(dc, base_x - data.button_size * 0.75, base_y + data.button_size * 0.75);
                        nvgLineTo(dc, base_x + data.button_size * 0.75, base_y - data.button_size * 0.75);
                        nvgStroke(dc);

                        nvgBeginPath(dc);
                        nvgStrokeWidth(dc, data.button_size * 0.3);
                        if (data.ptm == TWIXT_PP_PLAYER_WHITE) {
                            nvgStrokeColor(dc, nvgRGB(236, 236, 236));
                        } else {
                            nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        }
                        nvgMoveTo(dc, base_x - data.button_size * 0.7, base_y - data.button_size * 0.7);
                        nvgLineTo(dc, base_x + data.button_size * 0.7, base_y + data.button_size * 0.7);
                        nvgMoveTo(dc, base_x - data.button_size * 0.7, base_y + data.button_size * 0.7);
                        nvgLineTo(dc, base_x + data.button_size * 0.7, base_y - data.button_size * 0.7);
                        nvgStroke(dc);
                    } else {
                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                        nvgStrokeWidth(dc, data.button_size * 0.2);
                        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                        nvgStroke(dc);

                        nvgBeginPath(dc);
                        nvgCircle(dc, base_x, base_y, data.button_size - data.button_size * 0.1);
                        nvgStrokeWidth(dc, data.button_size * 0.15); // TODO maybe this thinner to increase contrast for white backline hovers
                        if (data.ptm == TWIXT_PP_PLAYER_WHITE) {
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

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "TwixT") == 0 && strcmp(methods->variant_name, "PP") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods twixt_pp_fem{
    .frontend_name = "twixt_pp",
    .version = semver{0, 2, 0},
    .features = frontend_feature_flags{
        .options = false,
    },

    .internal_methods = NULL,

    .opts_create = NULL,
    .opts_display = NULL,
    .opts_destroy = NULL,

    .get_last_error = get_last_error,

    .create = create,
    .destroy = destroy,

    .runtime_opts_display = runtime_opts_display,

    .process_event = process_event,
    .process_input = process_input,
    .update = update,

    .render = render,

    .is_game_compatible = is_game_compatible,

};
