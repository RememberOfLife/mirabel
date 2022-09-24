#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/tictactoe_ultimate.h"
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
        const tictactoe_ultimate_internal_methods* gi;
        uint8_t pbuf_c;
        player_id pbuf;
        float button_size;
        float local_padding;
        float global_padding;
        int mx;
        int my;
        sbtn board_buttons[9][9]; // board_buttons[y][x] origin is bottom left
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
            .button_size = 65,
            .local_padding = 3,
            .global_padding = 20,
            .mx = 0,
            .my = 0,
        };
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        data.board_buttons[8 - (gy * 3 + ly)][gx * 3 + lx] = sbtn{
                            static_cast<float>(gx) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(lx) * (data.button_size + data.local_padding),
                            static_cast<float>(gy) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(ly) * (data.button_size + data.local_padding),
                            data.button_size,
                            data.button_size,
                            false,
                            false,
                        };
                    }
                }
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
        ImGui::SliderFloat("button size", &data.button_size, 20, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("local padding", &data.local_padding, 0, 20, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("global padding", &data.global_padding, 0, 80, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        data_repr& data = _get_repr(self);
        bool dirty = false;
        switch (event.base.type) {
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
                data.gi = (const tictactoe_ultimate_internal_methods*)data.g.methods->internal_methods;
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
        //TODO
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
                    mX -= data.dd->w / 2 - (9 * data.button_size + 6 * data.local_padding + 2 * data.global_padding) / 2;
                    mY -= data.dd->h / 2 - (9 * data.button_size + 6 * data.local_padding + 2 * data.global_padding) / 2;
                    uint8_t global_target;
                    data.gi->get_global_target(&data.g, &global_target);
                    for (int gy = 0; gy < 3; gy++) {
                        for (int gx = 0; gx < 3; gx++) {
                            if (global_target != (((2 - gy) << 2) | gx) && global_target != ((3 << 2) | 3)) {
                                continue;
                            }
                            for (int ly = 0; ly < 3; ly++) {
                                for (int lx = 0; lx < 3; lx++) {
                                    int ix = gx * 3 + lx;
                                    int iy = 8 - (gy * 3 + ly);
                                    data.board_buttons[iy][ix].update(mX, mY);
                                    if (event.type == SDL_MOUSEBUTTONUP) {
                                        player_id cell_local;
                                        data.gi->get_cell_local(&data.g, ix, iy, &cell_local);
                                        if (data.board_buttons[iy][ix].hovered && data.board_buttons[iy][ix].mousedown && cell_local == 0) {
                                            uint64_t move_code = ix | (iy << 4);
                                            f_event_any es;
                                            f_event_create_game_move(&es, move_code);
                                            f_event_queue_push(data.dd->outbox, &es);
                                        }
                                        data.board_buttons[iy][ix].mousedown = false;
                                    }
                                    data.board_buttons[iy][ix].mousedown |= (data.board_buttons[iy][ix].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                                }
                            }
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
        mX -= data.dd->w / 2 - (9 * data.button_size + 6 * data.local_padding + 2 * data.global_padding) / 2;
        mY -= data.dd->h / 2 - (9 * data.button_size + 6 * data.local_padding + 2 * data.global_padding) / 2;
        uint8_t global_target;
        data.gi->get_global_target(&data.g, &global_target);
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                if (global_target != (((2 - gy) << 2) | gx) && global_target != ((3 << 2) | 3)) {
                    continue;
                }
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        int ix = gx * 3 + lx;
                        int iy = 8 - (gy * 3 + ly);
                        data.board_buttons[8 - (gy * 3 + ly)][gx * 3 + lx].x = static_cast<float>(gx) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(lx) * (data.button_size + data.local_padding);
                        data.board_buttons[8 - (gy * 3 + ly)][gx * 3 + lx].y = static_cast<float>(gy) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(ly) * (data.button_size + data.local_padding);
                        data.board_buttons[8 - (gy * 3 + ly)][gx * 3 + lx].w = data.button_size;
                        data.board_buttons[8 - (gy * 3 + ly)][gx * 3 + lx].h = data.button_size;
                        data.board_buttons[iy][ix].update(mX, mY);
                    }
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

        float local_board_size = 3 * data.button_size + 2 * data.local_padding;
        uint8_t global_target = 0;
        if (data.g.methods != NULL) {
            data.gi->get_global_target(&data.g, &global_target);
        }
        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, data.dd->w + 20, data.dd->h + 20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, data.dd->w / 2 - (3 * local_board_size + 2 * data.global_padding) / 2, data.dd->h / 2 - (3 * local_board_size + 2 * data.global_padding) / 2);
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                uint8_t local_result = 0;
                if (data.g.methods != NULL) {
                    data.gi->get_cell_global(&data.g, gx, 2 - gy, &local_result);
                }
                float base_x = gx * (local_board_size + data.global_padding);
                float base_y = gy * (local_board_size + data.global_padding);
                if (local_result > 0) {
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, local_board_size, local_board_size);
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                    nvgFill(dc);
                    nvgBeginPath(dc);
                    nvgStrokeWidth(dc, local_board_size * 0.175);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    switch (local_result) {
                        case 1: {
                            // X
                            nvgMoveTo(dc, base_x + local_board_size * 0.175, base_y + local_board_size * 0.175);
                            nvgLineTo(dc, base_x + local_board_size * 0.825, base_y + local_board_size * 0.825);
                            nvgMoveTo(dc, base_x + local_board_size * 0.175, base_y + local_board_size * 0.825);
                            nvgLineTo(dc, base_x + local_board_size * 0.825, base_y + local_board_size * 0.175);
                            nvgStroke(dc);
                        } break;
                        case 2: {
                            // O
                            nvgCircle(dc, base_x + local_board_size / 2, base_y + local_board_size / 2, local_board_size * 0.3);
                            nvgStroke(dc);
                        } break;
                        case 3: {
                            //TODO draw something for a draw
                        } break;
                    }
                    continue;
                }
                for (int ly = 0; ly < 3; ly++) {
                    for (int lx = 0; lx < 3; lx++) {
                        float base_x = static_cast<float>(gx) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(lx) * (data.button_size + data.local_padding);
                        float base_y = static_cast<float>(gy) * (3 * data.button_size + 2 * data.local_padding + data.global_padding) + static_cast<float>(ly) * (data.button_size + data.local_padding);
                        nvgStrokeWidth(dc, data.button_size * 0.175);
                        nvgBeginPath(dc);
                        nvgRect(dc, base_x, base_y, data.button_size, data.button_size);
                        if (data.g.methods != NULL && data.pbuf_c != 0 && (global_target == (((2 - gy) << 2) | gx) || global_target == ((3 << 2) | 3))) {
                            nvgFillColor(dc, nvgRGB(240, 217, 181));
                        } else {
                            nvgFillColor(dc, nvgRGB(161, 119, 67));
                        }
                        nvgFill(dc);
                        if (data.g.methods == NULL) {
                            continue;
                        }
                        int ix = gx * 3 + lx;
                        int iy = 8 - (gy * 3 + ly);
                        uint8_t player_in_cell;
                        data.gi->get_cell_local(&data.g, ix, iy, &player_in_cell);
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
                        } else if (data.board_buttons[iy][ix].hovered && data.pbuf_c > 0) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(220, 197, 161));
                            nvgRect(dc, data.board_buttons[iy][ix].x + data.button_size * 0.05, data.board_buttons[iy][ix].y + data.button_size * 0.05, data.board_buttons[iy][ix].w - data.button_size * 0.1, data.board_buttons[iy][ix].h - data.button_size * 0.1);
                            nvgFill(dc);
                        }
                        //TODO
                        /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((iy<<4)|ix)) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(125, 187, 248));
                            nvgCircle(dc, base_x+button_size/2, base_y+button_size/2, button_size*0.15);
                            nvgFill(dc);
                        }*/
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
        if (strcmp(methods->game_name, "TicTacToe") == 0 && strcmp(methods->variant_name, "Ultimate") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods tictactoe_ultimate_fem{
    .frontend_name = "tictactoe_ultimate",
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
