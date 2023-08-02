#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "mirabel/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    struct sbtn {
        float x;
        float y;
        float w;
        float h;
        bool hovered;
        bool mousedown;
        char* label; //TODO this might benefit from SSO

        union {
            player_id p;
            move_code m; //TODO will become big move later
        } v;

        void update(float mx, float my)
        {
            hovered = (mx >= x && mx <= x + w && my >= y && my <= y + h);
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;

        int mx;
        int my;

        float font_size;
        float xcol_offset;
        float xcol_spacing;
        float yrow_spacing;
        float btn_padding;
        float button_hmargin;

        game g;
        bool dirty;
        char* g_name;
        char* g_fflags;
        uint8_t g_pov_c;
        sbtn* g_pov_btns; //TODO offer way to also use redact information to just show what this player would know if wanted
        uint8_t g_pov_id;
        char* g_opts;
        //TODO legacy
        char* g_state;
        char* g_print;
        uint64_t g_id;
        // float* g_eval_f;
        // player_id* g_eval_p;
        uint8_t g_ptm_c;
        sbtn* g_ptm_btns;
        uint32_t g_moves_c;
        move_code* g_moves_buf;
        char* g_res_str;
    };

    data_repr& _get_repr(frontend* self)
    {
        return *((data_repr*)(self->data1));
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        self->data1 = malloc(sizeof(data_repr));
        data_repr& data = _get_repr(self);
        data = (data_repr){
            .dc = Control::main_client->nanovg_ctx,
            .dd = display_data,
            .mx = 0,
            .my = 0,
            .font_size = 20,
            .xcol_offset = 100,
            .xcol_spacing = 20,
            .yrow_spacing = 25,
            .btn_padding = 2,
            .button_hmargin = 4,
            .g = (game){
                .methods = NULL,
            },
            .dirty = false,
            .g_name = NULL,
            .g_fflags = NULL,
            .g_pov_c = 0,
            .g_pov_btns = NULL,
            .g_pov_id = PLAYER_NONE,
            .g_opts = NULL,
            .g_state = NULL,
            .g_print = NULL,
            .g_ptm_c = 0,
            .g_ptm_btns = NULL,
            .g_moves_c = 0,
            .g_moves_buf = NULL,
            .g_res_str = NULL,
        };
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        data_repr& data = _get_repr(self);
        event_any e1;
        event_create_type(&e1, EVENT_TYPE_GAME_UNLOAD);
        self->methods->process_event(self, e1);
        event_destroy(&e1);
        if (data.g.methods) {
            game_destroy(&data.g);
        }
        free(self->data1);
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        data_repr& data = _get_repr(self);
        ImGui::BeginDisabled();
        ImGui::SliderFloat("font size", &data.font_size, 10, 40); //TODO this doesnt actually work right now
        ImGui::EndDisabled();
        ImGui::SliderFloat("xcol offset", &data.xcol_offset, 50, 200);
        ImGui::SliderFloat("xcol spacing", &data.xcol_spacing, 10, 40);
        ImGui::SliderFloat("yrow spacing", &data.yrow_spacing, 10, 60);
        ImGui::SliderFloat("btn padding", &data.btn_padding, 0, 8);
        ImGui::SliderFloat("btn hmargin", &data.button_hmargin, 0, 10);
        //TODO offer color options
        //TODO offer imgui move making
        return ERR_OK;
    }

    error_code process_event(frontend* self, event_any event)
    {
        data_repr& data = _get_repr(self);
        switch (event.base.type) {
            case EVENT_TYPE_HEARTBEAT: {
                event_queue_push(data.dd->outbox, &event);
            } break;
            case EVENT_TYPE_GAME_LOAD_METHODS: {
                data.g.methods = event.game_load_methods.methods;
                data.g.data1 = NULL;
                data.g.data2 = NULL;
                data.g_name = (char*)malloc(strlen(game_gname(&data.g)) + strlen(game_vname(&data.g)) + strlen(game_iname(&data.g)) + 64);
                sprintf(data.g_name, "%s.%s.%s v%u.%u.%u", game_gname(&data.g), game_vname(&data.g), game_iname(&data.g), game_version(&data.g).major, game_version(&data.g).minor, game_version(&data.g).patch);
                {
                    data.g_fflags = (char*)malloc(32); // 23
                    sprintf(
                        data.g_fflags,
                        "%c%c%c%c%c%c%s %s %s %s %s %s %s %s",
                        game_ff(&data.g).error_strings ? 'E' : '-',
                        game_ff(&data.g).options ? 'O' : '-',
                        game_ff(&data.g).serializable ? 'S' : '-',
                        game_ff(&data.g).legacy ? 'L' : '-',
                        game_ff(&data.g).random_moves ? 'R' : '-',
                        game_ff(&data.g).hidden_information ? 'H' : '-',
                        game_ff(&data.g).simultaneous_moves ? "Sm" : "--",
                        game_ff(&data.g).big_moves ? "Bm" : "--",
                        game_ff(&data.g).move_ordering ? "Om" : "--",
                        game_ff(&data.g).scores ? "Sc" : "--",
                        game_ff(&data.g).id ? "Id" : "--",
                        game_ff(&data.g).eval ? "Ev" : "--",
                        game_ff(&data.g).playout ? "Rp" : "--",
                        game_ff(&data.g).print ? "Pr" : "--"
                        // game_ff(&data.g).time ? "T" : "-" //TODO add format string arg
                    );
                }
                game_create(&data.g, &event.game_load_methods.init_info);
                {
                    game_player_count(&data.g, &data.g_pov_c);
                    data.g_pov_c += 1; // for PLAYER_NONE
                    data.g_pov_btns = (sbtn*)malloc(sizeof(sbtn) * data.g_pov_c);
                    for (uint8_t i = 0; i < data.g_pov_c; i++) {
                        data.g_pov_btns[i] = (sbtn){.x = -100, .y = -100, .w = 0, .h = 0, .hovered = false, .mousedown = false};
                        data.g_pov_btns[i].label = (char*)malloc(8);
                        if (i == 0) {
                            sprintf(data.g_pov_btns[i].label, "NONE");
                            data.g_pov_btns[i].v.p = PLAYER_NONE;
                        } else {
                            sprintf(data.g_pov_btns[i].label, "%03hhu", i);
                            data.g_pov_btns[i].v.p = i;
                        }
                    }
                    data.g_pov_id = PLAYER_NONE;
                    data.dd->view = data.g_pov_id;
                }
                data.g_opts = NULL;
                if (game_ff(&data.g).options) {
                    size_t size_fill;
                    const char* gopts_local;
                    game_export_options(&data.g, PLAYER_NONE, &size_fill, &gopts_local);
                    data.g_opts = strdup(gopts_local);
                }
                data.g_state = NULL;
                data.g_print = NULL;
                {
                    // because pov_c already includes +1 for the NONE pov, we optionally reuse it later for the RAND ptm
                    data.g_ptm_c = 0;
                    data.g_ptm_btns = (sbtn*)malloc(sizeof(sbtn) * data.g_pov_c);
                    for (uint8_t i = 0; i < data.g_pov_c; i++) {
                        data.g_ptm_btns[i] = (sbtn){.x = -100, .y = -100, .w = 0, .h = 0, .hovered = false, .mousedown = false};
                        data.g_ptm_btns[i].label = (char*)malloc(8);
                    }
                }
                data.g_moves_c = 0;
                data.g_moves_buf = NULL;
                data.g_res_str = NULL;
                data.dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    game_destroy(&data.g);
                }
                data.g.methods = NULL;
                data.dirty = false;
                free(data.g_name);
                data.g_name = NULL;
                free(data.g_fflags);
                data.g_fflags = NULL;
                {
                    for (uint8_t i = 0; i < data.g_pov_c; i++) {
                        free(data.g_pov_btns[i].label);
                    }
                    free(data.g_pov_btns);
                    data.g_pov_btns = NULL;
                    data.g_pov_c = 0;
                    data.g_pov_id = PLAYER_NONE;
                    data.dd->view = data.g_pov_id;
                }
                free(data.g_opts);
                data.g_opts = NULL;
                free(data.g_state);
                data.g_state = NULL;
                free(data.g_print);
                data.g_print = NULL;
                {
                    for (uint8_t i = 0; i < data.g_ptm_c; i++) {
                        free(data.g_ptm_btns[i].label);
                    }
                    free(data.g_ptm_btns);
                    data.g_ptm_c = 0;
                    data.g_ptm_btns = NULL;
                }
                data.g_moves_c = 0;
                free(data.g_moves_buf);
                data.g_moves_buf = NULL;
                free(data.g_res_str);
                data.g_res_str = NULL;
            } break;
            case EVENT_TYPE_GAME_STATE: {
                game_import_state(&data.g, event.game_state.state);
                data.dirty = true;
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                game_make_move(&data.g, event.game_move.player, event.game_move.data);
                data.dirty = true;
            } break;
            default: {
                // pass
            } break;
        }
        event_destroy(&event);
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        data_repr& data = _get_repr(self);
        //BUG this can perform a click after previous window resizing in the same loop, while operating on the not yet updated button positions/sizes
        if (data.g.methods == NULL) {
            return ERR_OK;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                data.mx = event.motion.x - data.dd->x;
                data.my = event.motion.y - data.dd->y;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - data.dd->x;
                    int mY = event.button.y - data.dd->y;
                    // pov btns
                    for (uint8_t i = 0; i < data.g_pov_c; i++) {
                        data.g_pov_btns[i].update(mX, mY);
                        if (event.type == SDL_MOUSEBUTTONUP) {
                            if (data.g_pov_btns[i].hovered && data.g_pov_btns[i].mousedown) {
                                data.g_pov_id = i;
                                data.dd->view = data.g_pov_id;
                                data.dirty = true;
                            }
                            data.g_pov_btns[i].mousedown = false;
                        }
                        data.g_pov_btns[i].mousedown |= (data.g_pov_btns[i].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                    }
                    // ptm btns
                    for (uint8_t i = 0; i < data.g_ptm_c; i++) {
                        data.g_ptm_btns[i].update(mX, mY);
                        if (event.type == SDL_MOUSEBUTTONUP) {
                            if (data.g_ptm_btns[i].hovered && data.g_ptm_btns[i].mousedown) {
                                data.g_pov_id = data.g_ptm_btns[i].v.p;
                                data.dd->view = data.g_pov_id;
                                data.dirty = true;
                            }
                            data.g_ptm_btns[i].mousedown = false;
                        }
                        data.g_ptm_btns[i].mousedown |= (data.g_ptm_btns[i].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                    }
                }
            } break;
        }
        return ERR_OK;
    }

    error_code update(frontend* self)
    {
        data_repr& data = _get_repr(self);
        if (data.g.methods == NULL) {
            return ERR_OK;
        }
        if (data.dirty) {
            size_t size_fill;
            const char* str_buf;
            game_export_state(&data.g, data.g_pov_id, &size_fill, &str_buf);
            free(data.g_state);
            data.g_state = strdup(str_buf);
            if (game_ff(&data.g).print) {
                game_print(&data.g, data.g_pov_id, &size_fill, &str_buf);
                free(data.g_print);
                data.g_print = strdup(str_buf);
            }
            if (game_ff(&data.g).id) {
                game_id(&data.g, &data.g_id);
            }
            //TODO eval
            const player_id* player_buf;
            game_players_to_move(&data.g, &data.g_ptm_c, &player_buf);
            if (data.g_ptm_c > 0) {
                // update ptm soft buttons
                for (uint8_t i = 0; i < data.g_ptm_c; i++) {
                    if (player_buf[i] == PLAYER_ENV) {
                        sprintf(data.g_ptm_btns[i].label, "RAND");
                    } else {
                        sprintf(data.g_ptm_btns[i].label, "%03hhu", player_buf[i]);
                    }
                    data.g_ptm_btns[i].v.p = player_buf[i];
                }
                // generate moves if pov is to move
                if (data.g_pov_id != PLAYER_NONE) {
                    // check if pov is actually to move
                    for (uint8_t i = 0; i < data.g_ptm_c; i++) {
                        if (player_buf[i] == data.g_pov_id) {
                            //TODO generate moves for the player, need buf?
                            // game_get_concrete_moves(&data.g, data.g_pov_id, &data.g_moves_c, data.g_moves_buf);
                        }
                    }
                }
            }
            uint8_t u8_fill;
            game_get_results(&data.g, &u8_fill, &player_buf);
            free(data.g_res_str);
            data.g_res_str = NULL;
            if (u8_fill > 0) {
                data.g_res_str = (char*)malloc(u8_fill * 4);
                char* res_str = data.g_res_str;
                for (uint8_t i = 0; i < u8_fill; i++) {
                    res_str += sprintf(res_str, "%03hhu", player_buf[i]);
                    if (i < u8_fill - 1) {
                        res_str += sprintf(res_str, " ");
                    }
                }
            }
            data.dirty = false;
        }
        // simulate button placements to update positions
        NVGcontext* dc = data.dc;
        frontend_display_data& dd = *data.dd;
        const int I_XMIN = 0;
        const int I_YMIN = 1;
        const int I_XMAX = 2;
        const int I_YMAX = 3;
        float label_bounds[4]; // [xmin,ymin, xmax,ymax]
        int yrow = 1; // this is where the pov btns start
        float cumX;
        nvgBeginFrame(dc, dd.fbw, dd.fbh, 2); //TODO use proper devicePixelRatio
        nvgSave(dc);
        nvgFontSize(dc, data.font_size);
        nvgFontFace(dc, "mf");
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
        // pov buttons
        cumX = 0;
        for (uint8_t i = 0; i < data.g_pov_c; i++) {
            sbtn& btn = data.g_pov_btns[i];
            // set button hovered
            nvgTextBounds(dc, 0, 0, btn.label, NULL, label_bounds); //TODO fix buttons height extending too low
            btn.x = data.xcol_offset + cumX - data.btn_padding;
            btn.y = data.yrow_spacing * yrow - data.btn_padding;
            btn.w = label_bounds[I_XMAX] - label_bounds[I_XMIN] + data.btn_padding * 2;
            btn.h = label_bounds[I_YMAX] - label_bounds[I_YMIN] + data.btn_padding * 2;
            btn.update(data.mx, data.my);
            cumX += btn.w + data.button_hmargin;
        }
        yrow += 5;
        if (game_ff(&data.g).options) {
            yrow += 2;
        }
        yrow += 2;
        if (game_ff(&data.g).id) {
            yrow += 2;
        }
        // simulate multiline print
        if (game_ff(&data.g).print) {
            char* strstart = data.g_print;
            char* strend = strchr(strstart, '\n');
            while (strend != NULL) {
                yrow += 1;
                strstart = strend + 1;
                strend = strchr(strstart, '\n');
            }
            yrow += 1;
        }
        // ptm soft buttons
        cumX = 0;
        for (uint8_t i = 0; i < data.g_ptm_c; i++) {
            sbtn& btn = data.g_ptm_btns[i];
            // set button hovered
            nvgTextBounds(dc, 0, 0, btn.label, NULL, label_bounds); //TODO fix buttons height extending too low
            btn.x = data.xcol_offset + cumX - data.btn_padding;
            btn.y = data.yrow_spacing * yrow - data.btn_padding;
            btn.w = label_bounds[I_XMAX] - label_bounds[I_XMIN] + data.btn_padding * 2;
            btn.h = label_bounds[I_YMAX] - label_bounds[I_YMIN] + data.btn_padding * 2;
            btn.update(data.mx, data.my);
            cumX += btn.w + data.button_hmargin;
        }
        nvgRestore(dc);
        nvgEndFrame(dc);
        return ERR_OK;
    }

    void internal_draw_segment_liner(frontend* self, NVGcontext* dc, int yrow_prev, int yrow)
    {
        data_repr& data = _get_repr(self);
        nvgBeginPath(dc);
        nvgMoveTo(dc, data.xcol_offset - data.xcol_spacing / 2, data.yrow_spacing * yrow_prev - data.btn_padding);
        nvgLineTo(dc, data.xcol_offset - data.xcol_spacing / 2, data.yrow_spacing * (yrow - 2) + data.font_size + data.btn_padding);
        nvgStrokeColor(dc, nvgRGB(25, 25, 25));
        nvgStrokeWidth(dc, data.font_size * 0.1);
        nvgStroke(dc);
    }

    error_code render(frontend* self)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
        frontend_display_data& dd = *data.dd;

        nvgBeginFrame(dc, dd.fbw, dd.fbh, 2); //TODO use proper devicePixelRatio

        int yrow_prev = 0;
        int yrow = 1;

        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);

        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, dd.w + 20, dd.h + 20);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgFill(dc);

        nvgFontSize(dc, 20);
        nvgFontFace(dc, "mf");
        nvgFillColor(dc, nvgRGB(25, 25, 25));

        if (data.g.methods == NULL) {
            nvgRestore(dc);
            nvgEndFrame(dc);
            return ERR_OK;
        }

        yrow_prev = yrow;
        nvgBeginPath(dc);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
        nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "POV", NULL);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
        // render the pov buttons
        for (uint8_t i = 0; i < data.g_pov_c; i++) {
            sbtn& btn = data.g_pov_btns[i];
            nvgBeginPath(dc);
            nvgRect(dc, btn.x, btn.y, btn.w, btn.h);
            if (btn.v.p == data.g_pov_id) {
                nvgFillColor(dc, nvgRGB(119, 160, 213));
            } else if (btn.mousedown) {
                nvgFillColor(dc, nvgRGB(130, 130, 130));
            } else if (btn.hovered) {
                nvgFillColor(dc, nvgRGB(160, 160, 160));
            } else {
                nvgFillColor(dc, nvgRGB(180, 180, 180));
            }
            nvgFill(dc);

            //TODO some indicator on wether or not the POV is among the ptm or not
            nvgFillColor(dc, nvgRGB(25, 25, 25));
            nvgBeginPath(dc);
            nvgText(dc, btn.x + data.btn_padding, btn.y + data.btn_padding, btn.label, NULL);
        }
        yrow += 2;
        internal_draw_segment_liner(self, dc, yrow_prev, yrow);

        yrow_prev = yrow;
        nvgBeginPath(dc);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
        nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "NAME", NULL);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
        nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, data.g_name, NULL);
        yrow += 1;
        nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, data.g_fflags, NULL); //TODO show disabled flags in light gray or strikethrough
        yrow += 2;
        internal_draw_segment_liner(self, dc, yrow_prev, yrow);

        if (game_ff(&data.g).options) {
            yrow_prev = yrow;
            nvgBeginPath(dc);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "OPTS", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, data.g_opts, NULL);
            yrow += 2;
            internal_draw_segment_liner(self, dc, yrow_prev, yrow);
        }

        yrow_prev = yrow;
        nvgBeginPath(dc);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
        nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "STATE", NULL);
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
        nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, data.g_state, NULL); //TODO make sure this wraps if it goes on too long
        yrow += 2;
        internal_draw_segment_liner(self, dc, yrow_prev, yrow);

        if (game_ff(&data.g).id) {
            yrow_prev = yrow;
            char id_str[20];
            sprintf(id_str, "0x%016lx", data.g_id);
            nvgBeginPath(dc);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "ID", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, id_str, NULL);
            yrow += 2;
            internal_draw_segment_liner(self, dc, yrow_prev, yrow);
        }

        if (game_ff(&data.g).print) {
            yrow_prev = yrow;
            nvgBeginPath(dc);
            char* strstart = data.g_print;
            char* strend = strchr(strstart, '\n');
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "PRINT", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, strstart, strend);
            while (strend != NULL) {
                nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, strstart, strend);
                yrow += 1;
                strstart = strend + 1;
                strend = strchr(strstart, '\n');
            }
            yrow += 1;
            internal_draw_segment_liner(self, dc, yrow_prev, yrow);
        }

        // ptm and soft buttons that update pov on click
        if (data.g_ptm_c > 0) {
            yrow_prev = yrow;
            nvgBeginPath(dc);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "PTM", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            // render the ptm buttons
            for (uint8_t i = 0; i < data.g_ptm_c; i++) {
                sbtn& btn = data.g_ptm_btns[i];
                nvgBeginPath(dc);
                nvgRect(dc, btn.x, btn.y, btn.w, btn.h);
                if (btn.v.p == data.g_pov_id) {
                    nvgFillColor(dc, nvgRGB(156, 193, 241));
                } else if (btn.mousedown) {
                    nvgFillColor(dc, nvgRGB(130, 130, 130));
                } else if (btn.hovered) {
                    nvgFillColor(dc, nvgRGB(160, 160, 160));
                } else {
                    nvgFillColor(dc, nvgRGB(210, 210, 210));
                }
                nvgFill(dc);

                nvgFillColor(dc, nvgRGB(25, 25, 25));
                nvgBeginPath(dc);
                nvgText(dc, btn.x + data.btn_padding, btn.y + data.btn_padding, btn.label, NULL);
            }
            yrow += 2;
            internal_draw_segment_liner(self, dc, yrow_prev, yrow);
        }

        //TODO render all available moves for the selected ptm, or none at all if not selected

        if (data.g_res_str != NULL) {
            yrow_prev = yrow;
            nvgBeginPath(dc);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, data.xcol_offset - data.xcol_spacing, data.yrow_spacing * yrow, "RES", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            nvgText(dc, data.xcol_offset, data.yrow_spacing * yrow, data.g_res_str, NULL);
            yrow += 2;
            internal_draw_segment_liner(self, dc, yrow_prev, yrow);
        }

        nvgRestore(dc);
        nvgEndFrame(dc);
        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        return ERR_OK;
    }

} // namespace

const frontend_methods fallback_text_fem{
    .frontend_name = "fallback_text",
    .version = semver{1, 6, 3},
    .features = frontend_feature_flags{
        .error_strings = false,
        .options = false,
    },

    .internal_methods = NULL,

    .opts_create = NULL,
    .opts_display = NULL,
    .opts_destroy = NULL,

    .get_last_error = NULL,

    .create = create,
    .destroy = destroy,

    .runtime_opts_display = runtime_opts_display,

    .process_event = process_event,
    .process_input = process_input,
    .update = update,

    .render = render,

    .is_game_compatible = is_game_compatible,

};
