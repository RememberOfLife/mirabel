#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/tictactoe.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    //TODO display winning line
    //TODO button detection is one pixel to far to the top and left

    struct sbtn {
        float x;
        float y;
        float w;
        float h;
        bool hovered;
        bool mousedown;
        void update(float mx, float my)
        {
            hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        game g;
        const tictactoe_internal_methods* gi;
        uint8_t pbuf_c;
        player_id pbuf;
        float button_size;
        float padding;
        int mx;
        int my;
        sbtn board_buttons[3][3]; // board_buttons[y][x] origin is bottom left
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
            .dd = display_data,
            .g = (game){
                .methods = NULL,
            },
            .gi = NULL,
            .pbuf_c = 0,
            .pbuf = PLAYER_NONE,
            .button_size = 200,
            .padding = 20,
            .mx = 0,
            .my = 0,
        };
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                data.board_buttons[y][x] = sbtn{
                    static_cast<float>(x)*(data.button_size+data.padding),
                    (2*data.button_size+2*data.padding)-static_cast<float>(y)*(data.button_size+data.padding),
                    data.button_size, data.button_size, false, false
                };
            }
        }
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        free(self->data1);
        self->data1 = NULL;
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        data_repr& data = _get_repr(self);
        ImGui::SliderFloat("button size", &data.button_size, 20, 400, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &data.padding, 0, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        data_repr& data = _get_repr(self);
        bool dirty = false;
        switch(event.base.type) {
            case EVENT_TYPE_HEARTBEAT: {
                f_event_queue_push(data.dd->outbox, &event);
            } break;
            case EVENT_TYPE_GAME_LOAD_METHODS: {
                if (data.g.methods) {
                    data.g.methods->destroy(&data.g);
                }
                data.g.methods = event.game_load_methods.methods;
                data.g.data1 = NULL;
                data.g.data2 = NULL;
                data.g.methods->create_default(&data.g);
                data.gi = (const tictactoe_internal_methods*)data.g.methods->internal_methods;
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    data.g.methods->destroy(&data.g);
                }
                data.g.methods = NULL;
                dirty = false;
                data.gi = NULL;
            } break;
            case EVENT_TYPE_GAME_STATE: {
                data.g.methods->import_state(&data.g, event.game_state.state);
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                data.g.methods->make_move(&data.g, data.pbuf, event.game_move.code); //HACK //BUG need to use proper player to move, put it into move event
                dirty = true;
            } break;
            default: {
                // pass
            } break;
        }
        f_event_destroy(&event);
        if (dirty) {
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
            if (data.pbuf_c == 0) {
                uint8_t pres;
                data.g.methods->get_results(&data.g, &pres, &data.pbuf);
            }
        }
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        data_repr& data = _get_repr(self);
        //BUG this can perform a click after previous window resizing in the same loop, while operating on the not yet updated button positions/sizes
        if (data.g.methods == NULL || data.pbuf_c == 0) {
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
                    mX -= data.dd->w/2-(3*data.button_size+2*data.padding)/2;
                    mY -= data.dd->h/2-(3*data.button_size+2*data.padding)/2;
                    for (int x = 0; x < 3; x++) {
                        for (int y = 0; y < 3; y++) {
                            data.board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                player_id cell_player;
                                data.gi->get_cell(&data.g, x, y, &cell_player);
                                if (data.board_buttons[y][x].hovered && data.board_buttons[y][x].mousedown && cell_player == 0) {
                                    uint64_t move_code = x | (y<<2);
                                    f_event_any es;
                                    f_event_create_game_move(&es, move_code);
                                    f_event_queue_push(data.dd->outbox, &es);
                                }
                                data.board_buttons[y][x].mousedown = false;
                            }
                            data.board_buttons[y][x].mousedown |= (data.board_buttons[y][x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
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
        //TODO put button pos/size recalc into sdl resize event
        //TODO when reloading the game after a game is done, the hover does not reset
        if (data.g.methods == NULL || data.pbuf_c == 0) {
            return ERR_OK;
        }
        // set button hovered
        int mX = data.mx;
        int mY = data.my;
        mX -= data.dd->w/2-(3*data.button_size+2*data.padding)/2;
        mY -= data.dd->h/2-(3*data.button_size+2*data.padding)/2;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                data.board_buttons[y][x].x = static_cast<float>(x)*(data.button_size+data.padding);
                data.board_buttons[y][x].y = (2*data.button_size+2*data.padding)-static_cast<float>(y)*(data.button_size+data.padding);
                data.board_buttons[y][x].w = data.button_size;
                data.board_buttons[y][x].h = data.button_size;
                data.board_buttons[y][x].update(mX, mY);
            }
        }
        return ERR_OK;
    }
    
    error_code render(frontend* self)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
        frontend_display_data& dd = *data.dd;
        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);
        nvgStrokeWidth(dc, data.button_size*0.175);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, dd.w+20, dd.h+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, dd.w/2-(3*data.button_size+2*data.padding)/2, dd.h/2-(3*data.button_size+2*data.padding)/2);
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                float base_x = static_cast<float>(x)*(data.button_size+data.padding);
                float base_y = (2*data.button_size+2*data.padding)-static_cast<float>(y)*(data.button_size+data.padding);
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, data.button_size, data.button_size);
                if (data.g.methods == NULL || data.pbuf_c == 0) {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                } else {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                }
                nvgFill(dc);
                if (data.g.methods == NULL) {
                    continue;
                }
                player_id player_in_cell;
                data.gi->get_cell(&data.g, x, y, &player_in_cell);
                if (player_in_cell == 1) {
                    // X
                    nvgBeginPath(dc);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgMoveTo(dc, base_x+data.button_size*0.175, base_y+data.button_size*0.175);
                    nvgLineTo(dc, base_x+data.button_size*0.825, base_y+data.button_size*0.825);
                    nvgMoveTo(dc, base_x+data.button_size*0.175, base_y+data.button_size*0.825);
                    nvgLineTo(dc, base_x+data.button_size*0.825, base_y+data.button_size*0.175);
                    nvgStroke(dc);
                } else if (player_in_cell == 2) {
                    // O
                    nvgBeginPath(dc);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgCircle(dc, base_x+data.button_size/2, base_y+data.button_size/2, data.button_size*0.3);
                    nvgStroke(dc);
                } else if (data.board_buttons[y][x].hovered && data.pbuf > 0) {
                    nvgBeginPath(dc);
                    nvgFillColor(dc, nvgRGB(220, 197, 161));
                    nvgRect(dc, data.board_buttons[y][x].x+data.button_size*0.05, data.board_buttons[y][x].y+data.button_size*0.05, data.board_buttons[y][x].w-data.button_size*0.1, data.board_buttons[y][x].h-data.button_size*0.1);
                    nvgFill(dc);
                }
                //TODO
                /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((y<<2)|x)) {
                    nvgBeginPath(dc);
                    nvgFillColor(dc, nvgRGB(125, 187, 248));
                    nvgCircle(dc, board_buttons[y][x].x+data.button_size/2, board_buttons[y][x].y+data.button_size/2, data.button_size*0.15);
                    nvgFill(dc);
                }*/
            }
        }
        nvgRestore(dc);
        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "TicTacToe") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

}

const frontend_methods tictactoe_fem{
    .frontend_name = "tictactoe",
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
