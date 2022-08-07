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
/*
namespace Frontends {

    //TODO using this frontend has lots of double frees and other memory corruptions, and also crashed the sdl poll event somehow

    void FallbackText::set_game(game* new_game)
    {
        the_game = new_game;
        if (the_game) {
            pbuf = (player_id*)malloc(the_game->sizer.max_players_to_move);
            rbuf = (player_id*)malloc(the_game->sizer.max_results);
            if (the_game->methods->features.options) {
                opts_str = (char*)malloc(the_game->sizer.options_str);
            }
            state_str = (char*)malloc(the_game->sizer.state_str);
            if (the_game->methods->features.print) {
                print_str = (char*)malloc(the_game->sizer.print_str);
            }
        } else {
            free(pbuf);
            pbuf = NULL;
            pbuf_c = 0;
            free(rbuf);
            rbuf = NULL;
            rbuf_c = 0;
            free(opts_str);
            opts_str = NULL;
            free(state_str);
            state_str = NULL;
            free(print_str);
            print_str = NULL;
        }
    }

    void FallbackText::process_event(SDL_Event event)
    {}

    void FallbackText::update()
    {
        if (!the_game) {
            return;
        }
        if (the_game_step == Control::main_client->game_step) {
            return;
        }
        the_game_step = Control::main_client->game_step;
        size_t throwaway_size;
        the_game->methods->players_to_move(the_game, &pbuf_c, pbuf);
        if (pbuf_c == 0) {
            the_game->methods->get_results(the_game, &rbuf_c, rbuf);
        } else {
            rbuf_c = 0;
        }
        if (the_game->methods->features.options) {
            the_game->methods->export_options_str(the_game, &throwaway_size, opts_str);
        }
        the_game->methods->export_state(the_game, &throwaway_size, state_str);
        if (the_game->methods->features.id) {
            the_game->methods->id(the_game, &the_id);
        }
        if (the_game->methods->features.print) {
            the_game->methods->debug_print(the_game, &throwaway_size, print_str);
        }
    }

    void FallbackText::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgFill(dc);
        
        nvgFontSize(dc, 24);
        nvgFontFace(dc, "mf");
        
        nvgFillColor(dc, nvgRGB(25, 25, 25));
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);

        if (the_game) {
            //TODO prefix all the strings by a title what they are to their left, aligned top right

            if (the_game->methods->features.options) {
                nvgBeginPath(dc);
                nvgText(dc, 50, 50, opts_str, NULL);
            }
            nvgBeginPath(dc);
            nvgText(dc, 50, 100, state_str, NULL);
            if (the_game->methods->features.id) {
                char id_str[32];
                sprintf(id_str, "ID#%016lx", the_id);
                nvgBeginPath(dc);
                nvgText(dc, 50, 150, id_str, NULL);
            }
            if (the_game->methods->features.print) {
                nvgBeginPath(dc);
                //TODO print in multiple lines for newlines
                float lineoffset = 0;
                char* strstart = print_str;
                char* strend = strchr(strstart, '\n');
                nvgText(dc, 50, 200 + lineoffset, strstart, strend);
                // while (strend != NULL) {
                //     printf("%p %p\n", strstart, strend);
                //     lineoffset += 25;
                //     strstart = strend + 1;
                //     char* strend = strchr(strstart, '\n');
                // }
            }
        
            //TODO render all available moves and make them clickable
        
        }

        nvgRestore(dc);
    }


}
*/
namespace {

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* display;
        game g;
        char* g_opts;
        char* g_state;
        char* g_print;
        uint64_t g_id;
        float* g_eval_f;
        player_id* g_eval_p;
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
        data.dc = Control::main_client->nanovg_ctx;
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        free(self->data1);
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        //TODO
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        //TODO
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        //TODO
        return ERR_OK;
    }

    error_code update(frontend* self, player_id view)
    {
        //TODO
        return ERR_OK;
    }

    error_code background(frontend* self, float x, float y, float w, float h)
    {
        //TODO
        return ERR_OK;
    }

    error_code render(frontend* self, player_id view, float x, float y, float w, float h)
    {
        //TODO
        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        return ERR_OK;
    }

}

const frontend_methods fallback_text_fem{
    .frontend_name = "fallback_text",
    .version = semver{0, 1, 0},
    .features = frontend_feature_flags{
        .options = false,
        .global_background = true,
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

    .background = background,
    .render = render,

    .is_game_compatible = is_game_compatible,    

};
