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
            hovered = (mx >= x && mx <= x + w && my >= y && my <= y + h);
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        game g;
        const tictactoe_internal_methods* gi;
        player_id ptm = PLAYER_NONE;
        player_id res = PLAYER_NONE;
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
        data.dc = Control::main_client->nanovg_ctx;
        data.dd = display_data;
        data.g = (game){
            .methods = NULL,
        };
        data.gi = NULL;
        data.ptm = PLAYER_NONE;
        data.res = PLAYER_NONE;
        data.button_size = 200;
        data.padding = 20;
        data.mx = 0;
        data.my = 0;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                data.board_buttons[y][x] = sbtn{
                    static_cast<float>(x) * (data.button_size + data.padding),
                    (2 * data.button_size + 2 * data.padding) - static_cast<float>(y) * (data.button_size + data.padding),
                    data.button_size,
                    data.button_size,
                    false,
                    false,
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
                data.gi = (const tictactoe_internal_methods*)data.g.methods->internal_methods;
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    game_destroy(&data.g);
                }
                data.g.methods = NULL;
                dirty = false;
                data.gi = NULL;
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
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - data.dd->x;
                    int mY = event.button.y - data.dd->y;
                    mX -= data.dd->w / 2 - (3 * data.button_size + 2 * data.padding) / 2;
                    mY -= data.dd->h / 2 - (3 * data.button_size + 2 * data.padding) / 2;
                    for (int x = 0; x < 3; x++) {
                        for (int y = 0; y < 3; y++) {
                            data.board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                player_id cell_player;
                                data.gi->get_cell(&data.g, x, y, &cell_player);
                                if (data.board_buttons[y][x].hovered && data.board_buttons[y][x].mousedown && cell_player == 0) {
                                    uint64_t move_code = x | (y << 2);
                                    event_any es;
                                    event_create_game_move(&es, data.ptm, game_e_create_move_sync_small(&data.g, move_code));
                                    event_queue_push(data.dd->outbox, &es);
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
        if (data.g.methods == NULL || data.ptm == PLAYER_NONE) {
            return ERR_OK;
        }
        // set button hovered
        int mX = data.mx;
        int mY = data.my;
        mX -= data.dd->w / 2 - (3 * data.button_size + 2 * data.padding) / 2;
        mY -= data.dd->h / 2 - (3 * data.button_size + 2 * data.padding) / 2;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                data.board_buttons[y][x].x = static_cast<float>(x) * (data.button_size + data.padding);
                data.board_buttons[y][x].y = (2 * data.button_size + 2 * data.padding) - static_cast<float>(y) * (data.button_size + data.padding);
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

        nvgBeginFrame(dc, dd.fbw, dd.fbh, 2); //TODO use proper devicePixelRatio

        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);
        nvgStrokeWidth(dc, data.button_size * 0.175);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, dd.w + 20, dd.h + 20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, dd.w / 2 - (3 * data.button_size + 2 * data.padding) / 2, dd.h / 2 - (3 * data.button_size + 2 * data.padding) / 2);
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                float base_x = static_cast<float>(x) * (data.button_size + data.padding);
                float base_y = (2 * data.button_size + 2 * data.padding) - static_cast<float>(y) * (data.button_size + data.padding);
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, data.button_size, data.button_size);
                if (data.g.methods == NULL || data.ptm == PLAYER_NONE) {
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
                    nvgMoveTo(dc, base_x + data.button_size * 0.175, base_y + data.button_size * 0.175);
                    nvgLineTo(dc, base_x + data.button_size * 0.825, base_y + data.button_size * 0.825);
                    nvgMoveTo(dc, base_x + data.button_size * 0.175, base_y + data.button_size * 0.825);
                    nvgLineTo(dc, base_x + data.button_size * 0.825, base_y + data.button_size * 0.175);
                    nvgStroke(dc);
                } else if (player_in_cell == 2) {
                    // O
                    nvgBeginPath(dc);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgCircle(dc, base_x + data.button_size / 2, base_y + data.button_size / 2, data.button_size * 0.3);
                    nvgStroke(dc);
                } else if (data.board_buttons[y][x].hovered && data.ptm > PLAYER_NONE) {
                    nvgBeginPath(dc);
                    nvgFillColor(dc, nvgRGB(220, 197, 161));
                    nvgRect(dc, data.board_buttons[y][x].x + data.button_size * 0.05, data.board_buttons[y][x].y + data.button_size * 0.05, data.board_buttons[y][x].w - data.button_size * 0.1, data.board_buttons[y][x].h - data.button_size * 0.1);
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

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "TicTacToe") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

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
