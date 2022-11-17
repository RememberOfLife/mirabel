#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/havannah.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"
#include "meta_gui/meta_gui.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    void rotate_cords(float& x, float& y, float angle)
    {
        // rotate mouse input by +angle
        float pr = hypot(x, y);
        float pa = atan2(y, x) + angle;
        y = -pr * cos(pa);
        x = pr * sin(pa);
    }

    struct sbtn {
        float x;
        float y;
        float r;
        uint8_t ix;
        uint8_t iy;
        bool hovered;
        bool mousedown;

        void update(float mx, float my)
        {
            const float hex_angle = 2 * M_PI / 6;
            mx -= x;
            my -= y;
            // mouse is auto rotated by update to make this function assume global flat top
            // rotate in button space to make the collision work with the pointy top local hexes we get from global flat top
            rotate_cords(mx, my, hex_angle);
            mx = abs(mx);
            my = abs(my);
            // https://stackoverflow.com/questions/42903609/function-to-determine-if-point-is-inside-hexagon
            hovered = (my < std::round(sqrt(3) * std::min(r - mx, r / 2)));
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        game g = (game){
            .methods = NULL};
        const havannah_internal_methods* gi = NULL;

        uint8_t pbuf_c = 0;
        player_id pbuf = PLAYER_NONE;

        int size = 10;

        // runtime graphics options
        bool flat_top = false;
        float button_size = 25;
        float padding = 2.5;
        float stone_size_mult = 0.8;
        bool hex_stones = false;
        float connections_width = 0; //TODO same for virtual connections?

        int mx = 0;
        int my = 0;

        //TODO remove empty buttons and just malloc those actually required
        sbtn board_buttons[361]; // enough buttons for size 10
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
        new (self->data1) data_repr;
        data_repr& data = _get_repr(self);
        data.dc = Control::main_client->nanovg_ctx;
        data.dd = display_data;
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
        //TODO auto size
        ImGui::Checkbox("flat top", &data.flat_top);
        ImGui::SliderFloat("button size", &data.button_size, 10, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &data.padding, 0, 20, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("stone size", &data.stone_size_mult, 0.1, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::Checkbox("hex stones", &data.hex_stones);
        ImGui::SliderFloat("connections width", &data.connections_width, 0, 0.8, "%.3f", ImGuiSliderFlags_AlwaysClamp);
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
                    data.g.methods->destroy(&data.g);
                }
                data.g.methods = event.game_load_methods.methods;
                data.g.data1 = NULL;
                data.g.data2 = NULL;
                data.g.methods->create(&data.g, event.game_load_methods.init_info);
                data.gi = (const havannah_internal_methods*)data.g.methods->internal_methods;
                data.gi->get_size(&data.g, &data.size);
                dirty = true;
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (data.g.methods) {
                    data.g.methods->destroy(&data.g);
                }
                data.g.methods = NULL;
                data.gi = NULL;
                data.size = 10;
                dirty = false;
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
        event_destroy(&event);
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
                    const float hex_angle = 2 * M_PI / 6;
                    const float fitting_hex_radius = data.button_size + data.padding;
                    const float flat_radius = sin(hex_angle) * fitting_hex_radius;
                    int board_sizer = (2 * data.size - 1);
                    float mX = event.button.x - data.dd->x;
                    float mY = event.button.y - data.dd->y;
                    mX -= data.dd->w / 2;
                    mY -= data.dd->h / 2;
                    if (!data.flat_top) {
                        rotate_cords(mX, mY, hex_angle);
                    }
                    for (int y = 0; y < board_sizer; y++) {
                        for (int x = 0; x < board_sizer; x++) {
                            if (!(x - y < data.size) || !(y - x < data.size)) {
                                continue;
                            }
                            data.board_buttons[y * board_sizer + x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                HAVANNAH_PLAYER cp;
                                data.gi->get_cell(&data.g, x, y, &cp);
                                if (data.board_buttons[y * board_sizer + x].hovered && data.board_buttons[y * board_sizer + x].mousedown && cp == 0) {
                                    uint64_t move_code = y | (x << 8);
                                    event_any es;
                                    event_create_game_move(&es, EVENT_GAME_SYNC_DEFAULT, data.pbuf, move_code);
                                    event_queue_push(data.dd->outbox, &es);
                                }
                                data.board_buttons[y * board_sizer + x].mousedown = false;
                            }
                            data.board_buttons[y * board_sizer + x].mousedown |= (data.board_buttons[y * board_sizer + x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
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
        const float hex_angle = 2 * M_PI / 6;
        const float fitting_hex_radius = data.button_size + data.padding;
        const float flat_radius = sin(hex_angle) * fitting_hex_radius;
        int board_sizer = (2 * data.size - 1);
        float offset_x = -(data.size * flat_radius) + flat_radius;
        float offset_y = -((3 * data.size - 3) * fitting_hex_radius) / 2;
        float mX = data.mx;
        float mY = data.my;
        mX -= data.dd->w / 2;
        mY -= data.dd->h / 2;
        if (!data.flat_top) {
            // if global board is not flat topped, rotate the mouse so it is, for the collision check
            rotate_cords(mX, mY, hex_angle);
        }
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < data.size) || !(y - x < data.size)) {
                    continue;
                }
                int base_x_padding = (y + 1) / 2;
                float base_x = (x - base_x_padding) * (flat_radius * 2);
                if (y % 2 != 0) {
                    base_x += flat_radius;
                }
                float base_y = y * (fitting_hex_radius * 1.5);
                data.board_buttons[y * board_sizer + x].x = offset_x + base_x;
                data.board_buttons[y * board_sizer + x].y = offset_y + base_y;
                data.board_buttons[y * board_sizer + x].r = data.button_size;
                data.board_buttons[y * board_sizer + x].ix = x;
                data.board_buttons[y * board_sizer + x].iy = y;
                data.board_buttons[y * board_sizer + x].update(mX, mY);
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

        //TODO
        const float hex_angle = 2 * M_PI / 6;
        const float fitting_hex_radius = data.button_size + data.padding;
        const float flat_radius = sin(hex_angle) * fitting_hex_radius;
        int board_sizer = (2 * data.size - 1);
        nvgSave(dc);
        nvgTranslate(dc, dd.x, dd.y);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, dd.w + 20, dd.h + 20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, dd.w / 2, dd.h / 2);
        if (!data.flat_top) {
            nvgRotate(dc, hex_angle / 2);
        }
        // colored board border for current/winning player
        data.pbuf = HAVANNAH_PLAYER_NONE;
        nvgBeginPath(dc);
        if (data.g.methods == NULL) {
            nvgStrokeColor(dc, nvgRGB(161, 119, 67));
        } else {
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
            if (data.pbuf_c == 0) {
                data.g.methods->get_results(&data.g, &data.pbuf_c, &data.pbuf);
            }
            switch (data.pbuf) {
                default:
                case HAVANNAH_PLAYER_NONE: {
                    nvgStrokeColor(dc, nvgRGB(128, 128, 128));
                } break;
                case HAVANNAH_PLAYER_WHITE: {
                    nvgStrokeColor(dc, nvgRGB(141, 35, 35));
                } break;
                case HAVANNAH_PLAYER_BLACK: {
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                } break;
            }
            // actually we just want the ptm in there, so reste it back to that
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
        }
        nvgStrokeWidth(dc, flat_radius * 0.5);
        nvgMoveTo(dc, static_cast<float>(data.size * 2) * flat_radius, 0);
        for (int i = 0; i < 6; i++) {
            nvgRotate(dc, M_PI / 3);
            nvgLineTo(dc, static_cast<float>(data.size * 2) * flat_radius, 0);
        }
        nvgStroke(dc);
        // translate back up to board rendering position and render board
        nvgTranslate(dc, -(data.size * flat_radius) + flat_radius, -((3 * data.size - 3) * fitting_hex_radius) / 2);
        for (int y = 0; y < board_sizer; y++) {
            for (int x = 0; x < board_sizer; x++) {
                if (!(x - y < data.size) || !(y - x < data.size)) {
                    continue;
                }
                int base_x_padding = (y + 1) / 2;
                float base_x = (x - base_x_padding) * (flat_radius * 2);
                if (y % 2 != 0) {
                    base_x += flat_radius;
                }
                float base_y = y * (fitting_hex_radius * 1.5);
                nvgBeginPath(dc);
                if (data.g.methods == NULL || data.pbuf_c == 0) {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                } else {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                }
                nvgSave(dc);
                nvgTranslate(dc, base_x, base_y);
                nvgRotate(dc, hex_angle / 2);
                nvgMoveTo(dc, data.button_size, 0);
                for (int i = 0; i < 6; i++) {
                    nvgRotate(dc, M_PI / 3);
                    nvgLineTo(dc, data.button_size, 0);
                }
                nvgFill(dc);
                if (data.g.methods == NULL) {
                    nvgRestore(dc);
                    continue;
                }
                HAVANNAH_PLAYER cell_color;
                data.gi->get_cell(&data.g, x, y, &cell_color);
                switch (cell_color) {
                    case HAVANNAH_PLAYER_NONE: {
                        if (data.board_buttons[y * board_sizer + x].hovered && data.pbuf > HAVANNAH_PLAYER_NONE) {
                            nvgBeginPath(dc);
                            nvgFillColor(dc, nvgRGB(220, 197, 161));
                            nvgMoveTo(dc, data.button_size * 0.9, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI / 3);
                                nvgLineTo(dc, data.button_size * 0.9, 0);
                            }
                            nvgFill(dc);
                        }
                    } break;
                    case HAVANNAH_PLAYER_WHITE: {
                        nvgBeginPath(dc);
                        nvgFillColor(dc, nvgRGB(141, 35, 35));
                        if (data.hex_stones) {
                            nvgMoveTo(dc, data.button_size * data.stone_size_mult, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI / 3);
                                nvgLineTo(dc, data.button_size * data.stone_size_mult, 0);
                            }
                        } else {
                            nvgCircle(dc, 0, 0, data.button_size * data.stone_size_mult);
                        }
                        nvgFill(dc);
                    } break;
                    case HAVANNAH_PLAYER_BLACK: {
                        nvgBeginPath(dc);
                        nvgFillColor(dc, nvgRGB(25, 25, 25));
                        if (data.hex_stones) {
                            nvgMoveTo(dc, data.button_size * data.stone_size_mult, 0);
                            for (int i = 0; i < 6; i++) {
                                nvgRotate(dc, M_PI / 3);
                                nvgLineTo(dc, data.button_size * data.stone_size_mult, 0);
                            }
                        } else {
                            nvgCircle(dc, 0, 0, data.button_size * data.stone_size_mult);
                        }
                        nvgFill(dc);
                    } break;
                    case HAVANNAH_PLAYER_INVALID: {
                        assert(false);
                    } break;
                }
                // draw colored connections to same player pieces
                if (data.connections_width > 0 && cell_color != HAVANNAH_PLAYER_NONE) {
                    float connection_draw_width = data.button_size * data.connections_width;
                    // draw for self<->{1(x-1,y),3(x,y-1),2(x-1,y-1)}
                    uint8_t connections_to_draw = 0;
                    HAVANNAH_PLAYER cell_other;
                    data.gi->get_cell(&data.g, x - 1, y, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b001;
                    }
                    data.gi->get_cell(&data.g, x, y - 1, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b100;
                    }
                    data.gi->get_cell(&data.g, x - 1, y - 1, &cell_other);
                    if (cell_color == cell_other && cell_other != HAVANNAH_PLAYER_INVALID) {
                        connections_to_draw |= 0b010;
                    }
                    if (connections_to_draw) {
                        nvgSave(dc);
                        nvgRotate(dc, -M_PI / 6);
                        nvgBeginPath(dc);
                        switch (cell_color) {
                            case HAVANNAH_PLAYER_WHITE: {
                                nvgStrokeColor(dc, nvgRGB(141, 35, 35));
                            } break;
                            case HAVANNAH_PLAYER_BLACK: {
                                nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                            } break;
                            case HAVANNAH_PLAYER_NONE:
                            case HAVANNAH_PLAYER_INVALID: {
                                assert(false);
                            } break;
                        }
                        nvgRotate(dc, -M_PI - M_PI / 3);
                        for (int rot = 0; rot < 3; rot++) {
                            nvgRotate(dc, M_PI / 3);
                            if (!((connections_to_draw >> rot) & 0b1)) {
                                continue;
                            }
                            nvgRect(dc, -connection_draw_width / 2, -connection_draw_width / 2, connection_draw_width + flat_radius * 2, connection_draw_width);
                        }
                        nvgFill(dc);
                        nvgRestore(dc);
                    }
                }
                //TODO draw engine best move
                /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((x<<8)|y)) {
                    nvgStrokeColor(dc, nvgRGB(125, 187, 248));
                    nvgStrokeWidth(dc, data.button_size*0.1);
                    nvgMoveTo(dc, data.button_size*0.95, 0);
                    for (int i = 0; i < 6; i++) {
                        nvgRotate(dc, M_PI/3);
                        nvgLineTo(dc, data.button_size*0.95, 0);
                    }
                    nvgStroke(dc);
                }*/
                nvgRestore(dc);
            }
        }
        nvgRestore(dc);

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "Havannah") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods havannah_fem{
    .frontend_name = "havannah",
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
