#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* display;
        game g;
        bool dirty;
        char* g_opts;
        char* g_state;
        char* g_print;
        uint64_t g_id;
        // float* g_eval_f;
        // player_id* g_eval_p;
        player_id* g_ptm;
        uint8_t g_ptm_c;
        player_id* g_res;
        uint8_t g_res_c;
        move_code* g_moves;
        uint32_t g_moves_c;
    };

    data_repr& _get_repr(frontend* self)
    {
        return *((data_repr*)(self->data1));
    }

    const char* get_last_error(frontend* self)
    {
        //TODO
        return NULL;
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        self->data1 = malloc(sizeof(data_repr));
        data_repr& data = _get_repr(self);
        data = (data_repr){
            .dc = Control::main_client->nanovg_ctx,
            .display = display_data,
            .g = (game){
                .methods = NULL,
            },
            .dirty = false,
            .g_opts = NULL,
            .g_state = NULL,
            .g_print = NULL,
            .g_ptm = NULL,
            .g_ptm_c = 0,
            .g_res = NULL,
            .g_res_c = 0,
            .g_moves = NULL,
            .g_moves_c = 0,
        };
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        data_repr& data = _get_repr(self);
        if (data.g.methods) {
            data.g.methods->destroy(&data.g);
        }
        free(data.g_opts);
        data.g_opts = NULL;
        free(data.g_state);
        data.g_state = NULL;
        free(data.g_print);
        data.g_print = NULL;
        free(data.g_ptm);
        data.g_ptm = NULL;
        data.g_ptm_c = 0;
        free(data.g_res);
        data.g_res = NULL;
        data.g_res = 0;
        free(data.g_moves);
        data.g_moves = NULL;
        data.g_moves = 0;
        free(self->data1);
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        data_repr& data = _get_repr(self);
        //TODO offer color options
        //TODO offer imgui move making
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        data_repr& data = _get_repr(self);
        switch(event.base.type) {
            case EVENT_TYPE_HEARTBEAT: {
                f_event_queue_push(data.display->outbox, &event);
            } break;
            case EVENT_TYPE_GAME_LOAD_METHODS: {
                data.g.methods = event.game_load_methods.methods;
                data.g.data1 = NULL;
                data.g.data2 = NULL;
                if (data.g.methods->features.options) {
                    data.g.methods->create_with_opts_str(&data.g, event.game_load_methods.options);
                    data.g_opts = (char*)malloc(data.g.sizer.options_str);
                    size_t size_fill;
                    data.g.methods->export_options_str(&data.g, &size_fill, data.g_opts);
                } else {
                    data.g.methods->create_default(&data.g);
                }
                // allocate buffers
                data.g_state = (char*)malloc(data.g.sizer.state_str);
                if (data.g.methods->features.print) {
                    data.g_print = (char*)malloc(data.g.sizer.print_str);
                }
                data.g_ptm = (player_id*)malloc(sizeof(player_id) * data.g.sizer.max_players_to_move);
                data.g_ptm_c = 0;
                data.g_res = (player_id*)malloc(sizeof(player_id) * data.g.sizer.max_results);
                data.g_res = 0;
                data.g_moves = (move_code*)malloc(sizeof(move_code) * data.g.sizer.max_moves);
                data.g_moves = 0;
                data.dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    data.g.methods->destroy(&data.g);
                }
                data.g.methods = NULL;
                data.dirty = false;
                free(data.g_opts);
                data.g_opts = NULL;
                free(data.g_state);
                data.g_state = NULL;
                free(data.g_print);
                data.g_print = NULL;
                free(data.g_ptm);
                data.g_ptm = NULL;
                data.g_ptm_c = 0;
                free(data.g_res);
                data.g_res = NULL;
                data.g_res = 0;
                free(data.g_moves);
                data.g_moves = NULL;
                data.g_moves = 0;
            } break;
            case EVENT_TYPE_GAME_STATE: {
                data.g.methods->import_state(&data.g, event.game_state.state);
                data.dirty = true;
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                data.g.methods->make_move(&data.g, data.g_ptm[0], event.game_move.code); //HACK //BUG need to use proper player to move, put it into move event
                data.dirty = true;
            } break;
            default: {
                // pass
            } break;
        }
        f_event_destroy(&event);
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        data_repr& data = _get_repr(self);
        //TODO
        return ERR_OK;
    }

    error_code update(frontend* self, player_id view)
    {
        data_repr& data = _get_repr(self);
        if (data.dirty == false)  {
            return ERR_OK;
        }
        size_t size_fill;
        data.g.methods->export_state(&data.g, &size_fill, data.g_state);
        if (data.g.methods->features.print) {
            data.g.methods->debug_print(&data.g, &size_fill, data.g_print);
        }
        if (data.g.methods->features.id) {
            data.g.methods->id(&data.g, &data.g_id);
        }
        //TODO eval
        data.g.methods->players_to_move(&data.g, &data.g_ptm_c, data.g_ptm);
        data.g.methods->get_results(&data.g, &data.g_res_c, data.g_res);
        if (data.g_ptm_c > 0) {
            data.g.methods->get_concrete_moves(&data.g, data.g_ptm[0], &data.g_moves_c, data.g_moves); //HACK //BUG use proper ptm
        }
        data.dirty = false;
        return ERR_OK;
    }

    error_code render(frontend* self, player_id view, float x, float y, float w, float h)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;

        //TODO
        const float xcol_offset = 100;
        const float xcol_spacing = 20;
        const float yrow_spacing = 50;
        const float yline_spacing = 20;
        int yrow = 1;

        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, x+w+20, y+h+20);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgFill(dc);
        
        nvgFontSize(dc, 20);
        nvgFontFace(dc, "mf");
        
        nvgFillColor(dc, nvgRGB(25, 25, 25));

        if (data.g.methods) {
            //TODO draw line between title of line and the line, so if its multi line one can see what it belongs to

            if (data.g.methods->features.options) {
                nvgBeginPath(dc);
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
                nvgText(dc, xcol_offset - xcol_spacing, yrow_spacing * yrow, "OPTS", NULL);
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
                nvgText(dc, xcol_offset, yrow_spacing * yrow++, data.g_opts, NULL);
            }
            nvgBeginPath(dc);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
            nvgText(dc, xcol_offset - xcol_spacing, yrow_spacing * yrow, "STATE", NULL);
            nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
            nvgText(dc, xcol_offset, yrow_spacing * yrow++, data.g_state, NULL); //TODO make sure this wraps if it goes on too long
            if (data.g.methods->features.id) {
                char id_str[20];
                sprintf(id_str, "0x%016lx", data.g_id);
                nvgBeginPath(dc);
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
                nvgText(dc, xcol_offset - xcol_spacing, yrow_spacing * yrow, "ID", NULL);
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
                nvgText(dc, xcol_offset, yrow_spacing * yrow++, id_str, NULL);
            }
            if (data.g.methods->features.print) {
                nvgBeginPath(dc);
                float lineoffset = 0;
                char* strstart = data.g_print;
                char* strend = strchr(strstart, '\n');
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_RIGHT);
                nvgText(dc, xcol_offset - xcol_spacing, yrow_spacing * yrow, "PRINT", NULL);
                nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
                nvgText(dc, xcol_offset, yrow_spacing * yrow + lineoffset, strstart, strend);
                while (strend != NULL) {
                    nvgText(dc, xcol_offset, yrow_spacing * yrow + lineoffset, strstart, strend);
                    lineoffset += 25;
                    strstart = strend + 1;
                    strend = strchr(strstart, '\n');
                }
            }
        
            //TODO render all available moves and ptm and make them clickable

        }

        nvgRestore(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        return ERR_OK;
    }

}

const frontend_methods fallback_text_fem{
    .frontend_name = "fallback_text",
    .version = semver{1, 0, 0},
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
