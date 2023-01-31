#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/quasar.h"
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
        bool visible;

        void update(float mx, float my)
        {
            hovered = (mx >= x && mx <= x + w && my >= y && my <= y + h);
        }
    };

    static int BTNS_BIG_IDX = 0;
    static int BTNS_PAYOUT_IDX = 1;
    static int BTNS_SMALL_IDX = 2;

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        game g;
        const quasar_internal_methods* gi;
        player_id ptm = PLAYER_NONE;
        player_id res = PLAYER_NONE;
        float btn_size;
        float btn_padding;
        int mx;
        int my;
        sbtn btns[3];
    };

    data_repr& _get_repr(frontend* self)
    {
        return *((data_repr*)(self->data1));
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
        data.btn_size = 150;
        data.btn_padding = 20;
        data.mx = 0;
        data.my = 0;
        for (int i = 0; i < 0; i++) {
            data.btns[i] = sbtn{
                .x = (-(3 * data.btn_size + 2 * data.btn_padding) / 2) + i * data.btn_size + i * data.btn_padding,
                .y = 0,
                .w = data.btn_size,
                .h = data.btn_size,
                .hovered = false,
                .mousedown = false,
                .visible = false,
            };
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
        ImGui::SliderFloat("button size", &data.btn_size, 150, 500, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &data.btn_padding, 0, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
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
                data.gi = (const quasar_internal_methods*)data.g.methods->internal_methods;
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
            data.gi->can_big(&data.g, &data.btns[BTNS_BIG_IDX].visible);
            data.gi->can_payout(&data.g, &data.btns[BTNS_PAYOUT_IDX].visible);
            data.gi->can_small(&data.g, &data.btns[BTNS_SMALL_IDX].visible);
        }
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        data_repr& data = _get_repr(self);
        //TODO
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
                    mX -= data.dd->w / 2;
                    mY -= data.dd->h / 2;
                    for (int i = 0; i < 3; i++) {
                        data.btns[i].update(mX, mY);
                        if (data.btns[i].visible == false) {
                            data.btns[i].hovered = false;
                            data.btns[i].mousedown = false;
                            continue;
                        }
                        if (event.type == SDL_MOUSEBUTTONUP) {
                            if (data.btns[i].hovered && data.btns[i].mousedown) {
                                uint64_t move_code;
                                switch (i) {
                                    case 0: {
                                        move_code = QUASAR_MOVE_BIG;
                                    } break;
                                    case 1: {
                                        move_code = QUASAR_MOVE_PAYOUT;
                                    } break;
                                    case 2: {
                                        move_code = QUASAR_MOVE_SMALL;
                                    } break;
                                }
                                event_any es;
                                event_create_game_move(&es, data.ptm, game_e_create_move_sync_small(&data.g, move_code));
                                event_queue_push(data.dd->outbox, &es);
                            }
                            data.btns[i].mousedown = false;
                        }
                        data.btns[i].mousedown |= (data.btns[i].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                    }
                }
            } break;
        }
        return ERR_OK;
    }

    error_code update(frontend* self)
    {
        data_repr& data = _get_repr(self);
        // set button hovered
        int mX = data.mx;
        int mY = data.my;
        mX -= data.dd->w / 2;
        mY -= data.dd->h / 2;
        for (int i = 0; i < 3; i++) {
            data.btns[i].x = (-(3 * data.btn_size + 2 * data.btn_padding) / 2) + i * data.btn_size + i * data.btn_padding;
            data.btns[i].w = data.btn_size;
            data.btns[i].h = data.btn_size;
            if (data.g.methods == NULL || data.ptm == PLAYER_NONE) {
                data.btns[i].hovered = false;
                data.btns[i].mousedown = false;
                data.btns[i].visible = false;
            } else {
                data.btns[i].update(mX, mY);
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
        nvgRect(dc, -10, -10, data.dd->w + 20, data.dd->h + 20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, data.dd->w / 2, data.dd->h / 2);

        char num_label[8];
        if (data.g.methods) {
            uint8_t num;
            data.gi->get_num(&data.g, &num);
            sprintf(num_label, "%hhu", num);
        } else {
            sprintf(num_label, "X");
        }
        nvgFontSize(dc, data.btn_size * 1);
        nvgFontFace(dc, "ff");
        nvgFillColor(dc, nvgRGB(25, 25, 25));
        nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgText(dc, 0, -data.btn_padding, num_label, NULL);

        for (int i = 0; i < 3; i++) {
            nvgBeginPath(dc);
            nvgRect(dc, data.btns[i].x, data.btns[i].y, data.btns[i].w, data.btns[i].h);
            if (data.btns[i].visible) {
                nvgFillColor(dc, nvgRGB(240, 217, 181));
            } else {
                nvgFillColor(dc, nvgRGB(161, 119, 67));
            }
            nvgFill(dc);
            if (data.btns[i].visible || (data.ptm == PLAYER_NONE && i == BTNS_PAYOUT_IDX)) {
                if (data.btns[i].mousedown) {
                    nvgBeginPath(dc);
                    nvgRect(dc, data.btns[i].x + data.btn_size * 0.05, data.btns[i].y + data.btn_size * 0.05, data.btns[i].w - data.btn_size * 0.1, data.btns[i].h - data.btn_size * 0.1);
                    nvgFillColor(dc, nvgRGB(200, 177, 141));
                    nvgFill(dc);
                } else if (data.btns[i].hovered) {
                    nvgBeginPath(dc);
                    nvgRect(dc, data.btns[i].x + data.btn_size * 0.05, data.btns[i].y + data.btn_size * 0.05, data.btns[i].w - data.btn_size * 0.1, data.btns[i].h - data.btn_size * 0.1);
                    nvgFillColor(dc, nvgRGB(220, 197, 161));
                    nvgFill(dc);
                }
                //TODO double line text, or symbols instead?
                char btn_label[8];
                switch (i) {
                    case 0: {
                        sprintf(btn_label, "1-8");
                    } break;
                    case 1: {
                        if (data.g.methods) {
                            int score;
                            data.gi->get_score(&data.g, &score);
                            sprintf(btn_label, "%i", score);
                        } else {
                            btn_label[0] = '\0';
                        }
                    } break;
                    case 2: {
                        sprintf(btn_label, "4-7");
                    } break;
                }
                nvgFontSize(dc, data.btn_size * 0.3);
                nvgFontFace(dc, "ff");
                nvgFillColor(dc, nvgRGB(25, 25, 25));
                nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(dc, data.btns[i].x + data.btns[i].w / 2, data.btns[i].y + data.btns[i].h / 2, btn_label, NULL);
            }
        }

        nvgRestore(dc);

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "Quasar") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods quasar_fem{
    .frontend_name = "quasar",
    .version = semver{0, 1, 3},
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