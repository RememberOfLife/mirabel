#include <cstdint>
#include <unordered_map>
#include <vector>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/games/chess.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"
#include "meta_gui/meta_gui.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    //TODO display when you king is in check, just correctly set data.check
    //TODO draw arrows and markers with right click
    //TODO highlist last made mode

    struct sbtn {
        float x;
        float y;
        float s;
        bool hovered;

        void update(float mx, float my)
        {
            hovered = (mx >= x && mx <= x + s && my >= y && my <= y + s);
        }
    };

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;

        game g = (game){
            .methods = NULL};
        const chess_internal_methods* gi = NULL;

        uint8_t pbuf_c = 0;
        player_id pbuf = PLAYER_NONE;
        uint32_t move_cnt = 0;
        move_code moves[CHESS_MAX_MOVES];
        player_id check = PLAYER_NONE;

        float square_size = 90;
        bool auto_size = true;
        bool promotion_auto_queen = false;

        int sprites[12];

        int mx = 0;
        int my = 0;

        sbtn board_buttons[8][8]; // board_buttons[y][x] origin is bottom left

        std::unordered_map<uint8_t, std::vector<uint8_t>> move_map;
        bool passive_pin = false;
        bool hover_outside_of_pin = false;
        int mouse_pindx_x = -1;
        int mouse_pindx_y = -1;

        sbtn promotion_buttons[2][2];
        bool promotion_buttons_mdown = false;
        int promotion_ox = -1;
        int promotion_oy = -1;
        int promotion_tx = -1;
        int promotion_ty = -1;
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
        // load sprites from res folder
        for (int i = 0; i < 12; i++) {
            data.sprites[i] = -1;
        }
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_KING - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_QUEEN - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_ROOK - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_BISHOP - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_KNIGHT - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_WHITE * 6 - 6 + CHESS_PIECE_TYPE_PAWN - 1] = nvgCreateImage(data.dc, "../res/games/chess/pwp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_KING - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_QUEEN - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_ROOK - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_BISHOP - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_KNIGHT - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        data.sprites[CHESS_PLAYER_BLACK * 6 - 6 + CHESS_PIECE_TYPE_PAWN - 1] = nvgCreateImage(data.dc, "../res/games/chess/pbp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        for (int i = 0; i < 12; i++) {
            if (data.sprites[i] < 0) {
                MetaGui::logf("#E chess: sprite loading failure #%d\n", i);
                //TODO any backup?
            }
        }
        // setup buttons for drag and drop
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                data.board_buttons[y][x] = sbtn{
                    static_cast<float>(x) * (data.square_size),
                    (7 * data.square_size) - static_cast<float>(y) * (data.square_size),
                    data.square_size,
                    false};
            }
        }
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        data_repr& data = _get_repr(self);
        for (int i = 0; i < 12; i++) {
            nvgDeleteImage(data.dc, data.sprites[i]);
        }
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
        ImGui::SliderFloat("size", &data.square_size, 40, 175, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        if (data.auto_size) {
            ImGui::EndDisabled();
        }
        ImGui::Checkbox("promotion auto queen", &data.promotion_auto_queen);
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
                data.gi = (const chess_internal_methods*)data.g.methods->internal_methods;
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
        event_destroy(&event);
        if (dirty) {
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
            data.move_map.clear();
            if (data.pbuf_c == 0) {
                uint8_t pres;
                data.g.methods->get_results(&data.g, &pres, &data.pbuf);
            } else {
                data.g.methods->get_concrete_moves(&data.g, data.pbuf, &data.move_cnt, data.moves);
                for (int i = 0; i < data.move_cnt; i++) {
                    uint8_t m_from = (data.moves[i] >> 8) & 0xFF;
                    if (data.move_map.find(m_from) == data.move_map.end()) {
                        data.move_map.emplace(m_from, std::vector<uint8_t>{});
                    }
                    uint8_t m_to = data.moves[i] & 0xFF;
                    data.move_map.at(m_from).push_back(m_to);
                }
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
                    uint64_t target_move = MOVE_NONE;
                    bool new_pickup = false;
                    // is proper left mouse button down event
                    int mX = event.button.x - data.dd->x;
                    int mY = event.button.y - data.dd->y;
                    mX -= data.dd->w / 2 - (8 * data.square_size) / 2;
                    mY -= data.dd->h / 2 - (8 * data.square_size) / 2;
                    // check if promotion menu is active and interacted with
                    if (data.promotion_tx >= 0) {
                        for (int py = 0; py < 2; py++) {
                            for (int px = 0; px < 2; px++) {
                                if (event.type == SDL_MOUSEBUTTONUP) {
                                    data.promotion_buttons[py][px].update(mX, mY);
                                    if (data.promotion_buttons[py][px].hovered && data.promotion_buttons_mdown) {
                                        int promotion_type = py * 2 + px + 2;
                                        target_move = (promotion_type << 16) | (data.promotion_ox << 12) | (data.promotion_oy << 8) | (data.promotion_tx << 4) | (data.promotion_ty);
                                    }
                                }
                            }
                        }
                        data.board_buttons[data.promotion_ty][data.promotion_tx].update(mX, mY);
                        data.promotion_buttons_mdown |= (data.board_buttons[data.promotion_ty][data.promotion_tx].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        if (!data.passive_pin && (event.type == SDL_MOUSEBUTTONUP && target_move == MOVE_NONE) || (event.type == SDL_MOUSEBUTTONDOWN && !data.promotion_buttons_mdown)) {
                            data.promotion_ox = -1;
                            data.promotion_oy = -1;
                            data.promotion_tx = -1;
                            data.promotion_ty = -1;
                            data.promotion_buttons_mdown = false;
                        }
                        data.passive_pin = false;
                    }
                    if (data.promotion_tx < 0) {
                        for (int y = 0; y < 8; y++) {
                            for (int x = 0; x < 8; x++) {
                                if (target_move != MOVE_NONE) {
                                    continue;
                                }
                                data.board_buttons[y][x].update(mX, mY);
                                if (event.type == SDL_MOUSEBUTTONDOWN) {
                                    // on down event, pickup piece that is hovered, if any
                                    CHESS_piece sp;
                                    data.gi->get_cell(&data.g, x, y, &sp);
                                    if (data.passive_pin && data.board_buttons[y][x].hovered) {
                                        if (sp.player == data.pbuf) {
                                            // if this pin was already pinned, then assume we hovered outside, so we can put it down on mouse up again
                                            data.hover_outside_of_pin = (data.mouse_pindx_x == x && data.mouse_pindx_y == y);
                                            // new pickup, drop old piece that was passive pinned
                                            data.passive_pin = false;
                                            new_pickup = true;
                                            data.mouse_pindx_x = x;
                                            data.mouse_pindx_y = y;
                                        } else {
                                            // if a pinned piece is set, instead MOVE it to the mousedown location
                                            target_move = (data.mouse_pindx_x << 12) | (data.mouse_pindx_y << 8) | (x << 4) | (y);
                                            // do not set new pickup here, so that the pin gets cleared automatically
                                        }
                                    } else if (data.board_buttons[y][x].hovered && sp.type != CHESS_PIECE_TYPE_NONE && sp.player == data.pbuf) {
                                        // new pickup
                                        data.hover_outside_of_pin = false;
                                        new_pickup = true;
                                        data.mouse_pindx_x = x;
                                        data.mouse_pindx_y = y;
                                    }
                                }
                                if (event.type == SDL_MOUSEBUTTONUP) {
                                    if (data.board_buttons[y][x].hovered) {
                                        // dropped piece onto a square
                                        if (x == data.mouse_pindx_x && y == data.mouse_pindx_y) {
                                            // dropped back onto the picked up piece
                                            data.passive_pin = true;
                                            // if at least hovered outside of this
                                            if (data.hover_outside_of_pin) {
                                                data.passive_pin = false;
                                                data.mouse_pindx_x = -1;
                                                data.mouse_pindx_y = -1;
                                            }
                                        } else {
                                            // dropped onto another square
                                            target_move = (data.mouse_pindx_x << 12) | (data.mouse_pindx_y << 8) | (x << 4) | (y);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (event.type == SDL_MOUSEBUTTONDOWN && !new_pickup) {
                        data.passive_pin = false;
                        data.mouse_pindx_x = -1;
                        data.mouse_pindx_y = -1;
                    }
                    // reset pin idx after processing of a click
                    if (event.type == SDL_MOUSEBUTTONUP && !data.passive_pin) {
                        data.mouse_pindx_x = -1;
                        data.mouse_pindx_y = -1;
                    }
                    if (target_move != MOVE_NONE) {
                        // check if this move would be a promotion
                        int ox = (target_move >> 12) & 0xF;
                        int oy = (target_move >> 8) & 0xF;
                        int tx = (target_move >> 4) & 0xF;
                        int ty = target_move & 0xF;
                        int pt = (target_move >> 16) & 0xF;
                        CHESS_piece sp;
                        data.gi->get_cell(&data.g, ox, oy, &sp);
                        if (sp.type == CHESS_PIECE_TYPE_PAWN && (ty == 0 || ty == 7)) {
                            if (pt > CHESS_PIECE_TYPE_NONE) {
                                // reset promotion menu, this is already a valid promotion move
                                data.promotion_ox = -1;
                                data.promotion_oy = -1;
                                data.promotion_tx = -1;
                                data.promotion_ty = -1;
                                data.promotion_buttons_mdown = false;
                            } else if (data.promotion_auto_queen) {
                                target_move |= (CHESS_PIECE_TYPE_QUEEN << 16);
                            } else {
                                // open promotion menu on the target square, only if the move would be legal
                                bool open_promotion = false;
                                data.g.methods->get_concrete_moves(&data.g, data.pbuf, &data.move_cnt, data.moves);
                                for (int i = 0; i < data.move_cnt; i++) {
                                    if ((data.moves[i] & 0xFFFF) == target_move) {
                                        open_promotion = true;
                                        break;
                                    }
                                }
                                if (open_promotion) {
                                    data.promotion_ox = ox;
                                    data.promotion_oy = oy;
                                    data.promotion_tx = tx;
                                    data.promotion_ty = ty;
                                    data.passive_pin = true;
                                }
                            }
                        }
                        for (int i = 0; i < data.move_cnt; i++) {
                            if (data.moves[i] == target_move) {
                                event_any es;
                                event_create_game_move(&es, target_move);
                                event_queue_push(data.dd->outbox, &es);
                                break;
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
        if (data.auto_size) {
            data.square_size = (data.dd->h < data.dd->w ? data.dd->h : data.dd->w) / 8.75;
        }
        //TODO put button pos/size recalc into sdl resize event
        //TODO when reloading the game after a game is done, the hover does not reset
        if (data.g.methods == NULL || data.pbuf_c == 0) {
            return ERR_OK;
        }
        // set button hovered
        int mX = data.mx;
        int mY = data.my;
        mX -= data.dd->w / 2 - (8 * data.square_size) / 2;
        mY -= data.dd->h / 2 - (8 * data.square_size) / 2;
        if (data.promotion_tx >= 0) {
            for (int y = 0; y < 2; y++) {
                for (int x = 0; x < 2; x++) {
                    data.promotion_buttons[y][x].x = (data.square_size * data.promotion_tx) + (x * data.square_size / 2);
                    data.promotion_buttons[y][x].y = ((7 * data.square_size) - (data.square_size * data.promotion_ty)) + (y * data.square_size / 2);
                    data.promotion_buttons[y][x].s = data.square_size / 2;
                    data.promotion_buttons[y][x].update(mX, mY);
                }
            }
        }
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                data.board_buttons[y][x].x = static_cast<float>(x) * (data.square_size);
                data.board_buttons[y][x].y = (7 * data.square_size) - static_cast<float>(y) * (data.square_size);
                data.board_buttons[y][x].s = data.square_size;
                data.board_buttons[y][x].update(mX, mY);
                if (data.board_buttons[y][x].hovered && (data.mouse_pindx_x != x || data.mouse_pindx_y != y)) {
                    data.hover_outside_of_pin |= true;
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
        nvgTranslate(dc, dd.w / 2 - (8 * data.square_size) / 2, dd.h / 2 - (8 * data.square_size) / 2);
        // colored board border for current/winning player
        float border_size = data.square_size * 0.1;
        nvgBeginPath(dc);
        nvgStrokeWidth(dc, border_size);
        nvgRect(dc, -border_size, -border_size, 8 * data.square_size + 2 * border_size, 8 * data.square_size + 2 * border_size);
        if (data.g.methods == NULL) {
            nvgStrokeColor(dc, nvgRGB(128, 128, 128));
        } else {
            CHESS_PLAYER color_player = CHESS_PLAYER_NONE;
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
            if (data.pbuf_c == 0) {
                data.g.methods->get_results(&data.g, &data.pbuf_c, &data.pbuf);
                nvgRect(dc, -2.5 * border_size, -2.5 * border_size, 8 * data.square_size + 5 * border_size, 8 * data.square_size + 5 * border_size);
            }
            color_player = (CHESS_PLAYER)data.pbuf;
            switch (color_player) {
                case CHESS_PLAYER_NONE: {
                    nvgStrokeColor(dc, nvgRGB(128, 128, 128));
                } break;
                case CHESS_PLAYER_WHITE: {
                    nvgStrokeColor(dc, nvgRGB(236, 236, 236));
                } break;
                case CHESS_PLAYER_BLACK: {
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                } break;
            }
            // actually we just want the ptm in there, so reste it back to that
            data.g.methods->players_to_move(&data.g, &data.pbuf_c, &data.pbuf);
        }
        nvgStroke(dc);
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int iy = 7 - y;
                float base_x = x * data.square_size;
                float base_y = y * data.square_size;
                // draw base square
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, data.square_size, data.square_size);
                nvgFillColor(dc, ((x + y) % 2 == 0) ? nvgRGB(240, 217, 181) : nvgRGB(161, 119, 67));
                nvgFill(dc);
                if (data.promotion_tx == x && data.promotion_ty == iy) {
                    // draw the promotion menu here
                    float half_sqsize = data.square_size / 2;
                    for (int py = 0; py < 2; py++) {
                        for (int px = 0; px < 2; px++) {
                            nvgBeginPath(dc);
                            nvgRect(dc, base_x + px * half_sqsize, base_y + py * half_sqsize, half_sqsize, half_sqsize);
                            int promotion_type = py * 2 + px + 2;
                            int promotion_sprite_idx = data.pbuf * 6 - 6 + promotion_type - 1;
                            NVGpaint promotion_sprite_paint = nvgImagePattern(dc, base_x + px * half_sqsize, base_y + py * half_sqsize, half_sqsize, half_sqsize, 0, data.sprites[promotion_sprite_idx], 0.5);
                            nvgFillPaint(dc, promotion_sprite_paint);
                            nvgFill(dc);
                            if (data.promotion_buttons[py][px].hovered) {
                                //TODO gets rendered UNDER the rank-file notation
                                nvgBeginPath(dc);
                                nvgRect(dc, base_x + px * data.square_size / 2, base_y + py * data.square_size / 2, half_sqsize, half_sqsize);
                                nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                                nvgFill(dc);
                            }
                        }
                    }
                }
                // draw rank and file
                static char char_buf[2] = "-";
                float text_padding = data.square_size * 0.05;
                nvgFontSize(dc, data.square_size * 0.17);
                nvgFontFace(dc, "ff");
                nvgFillColor(dc, ((x + y) % 2 != 0) ? nvgRGB(240, 217, 181) : nvgRGB(161, 119, 67)); // flip color from base square
                if (x == 7) {
                    char_buf[0] = '1' + iy;
                    nvgBeginPath(dc);
                    nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
                    nvgText(dc, base_x + data.square_size - text_padding, base_y + text_padding, char_buf, NULL);
                }
                if (iy == 0) {
                    char_buf[0] = 'a' + x;
                    nvgBeginPath(dc);
                    nvgTextAlign(dc, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
                    nvgText(dc, base_x + text_padding, base_y + data.square_size - text_padding, char_buf, NULL);
                }
                if (data.g.methods == NULL) {
                    continue;
                }
                if (data.promotion_ox == x && data.promotion_oy == iy) {
                    // skip if the pawn here is held up in a promotion menu, draw green background to indicate its involvement
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, data.square_size, data.square_size);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                    continue;
                }
                if (data.promotion_tx == x && data.promotion_ty == iy) {
                    continue;
                }
                CHESS_piece piece_in_square;
                data.gi->get_cell(&data.g, x, iy, &piece_in_square);
                if (piece_in_square.player == CHESS_PLAYER_NONE) {
                    // skip if empty
                    continue;
                }
                float sprite_alpha = 1;
                if (x == data.mouse_pindx_x && iy == data.mouse_pindx_y) {
                    sprite_alpha = 0.5;
                    // render green underlay under the picked up piece
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, data.square_size, data.square_size);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                }
                if (piece_in_square.type == CHESS_PIECE_TYPE_KING && piece_in_square.player == data.check) {
                    // render red capture type pattern under kings in check
                    float s = data.square_size * 0.3;
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + s, base_y);
                    nvgLineTo(dc, base_x, base_y + s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x + data.square_size, base_y);
                    nvgLineTo(dc, base_x + data.square_size - s, base_y);
                    nvgLineTo(dc, base_x + data.square_size, base_y + s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x, base_y + data.square_size);
                    nvgLineTo(dc, base_x, base_y + data.square_size - s);
                    nvgLineTo(dc, base_x + s, base_y + data.square_size);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x + data.square_size, base_y + data.square_size);
                    nvgLineTo(dc, base_x + data.square_size - s, base_y + data.square_size);
                    nvgLineTo(dc, base_x + data.square_size, base_y + data.square_size - s);
                    nvgClosePath(dc);
                    nvgFillColor(dc, nvgRGBA(200, 24, 24, 192));
                    nvgFill(dc);
                }
                // render the piece sprites
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, data.square_size, data.square_size);
                int sprite_idx = piece_in_square.player * 6 - 6 + piece_in_square.type - 1;
                NVGpaint sprite_paint = nvgImagePattern(dc, base_x, base_y, data.square_size, data.square_size, 0, data.sprites[sprite_idx], sprite_alpha);
                nvgFillPaint(dc, sprite_paint);
                nvgFill(dc);
            }
        }
        // render possible moves, if pinned piece exists
        uint8_t m_from = (data.mouse_pindx_x << 4) | (data.mouse_pindx_y);
        if (data.mouse_pindx_x >= 0 && data.move_map.find(m_from) != data.move_map.end()) {
            uint64_t drawn_bitboard = 0;
            std::vector<uint8_t> moves = data.move_map.at(m_from);
            for (int i = 0; i < moves.size(); i++) {
                int ix = (moves[i] >> 4) & 0x0F;
                int iy = (moves[i] & 0x0F);

                uint64_t bitboard_mask = 1;
                bitboard_mask <<= iy * 8 + ix;
                if (drawn_bitboard & bitboard_mask) {
                    continue; // prevent promotion moves from being draw on top of each other
                }
                drawn_bitboard |= bitboard_mask;

                float base_x = ix * data.square_size;
                float base_y = (7 - iy) * data.square_size;
                CHESS_piece sp;
                data.gi->get_cell(&data.g, ix, iy, &sp);
                if (data.board_buttons[iy][ix].hovered) {
                    // currently hovering the possible move
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, data.square_size, data.square_size);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                } else if (sp.type == CHESS_PIECE_TYPE_NONE) {
                    // is a move to empty square
                    nvgBeginPath(dc);
                    nvgCircle(dc, base_x + data.square_size / 2, base_y + data.square_size / 2, data.square_size * 0.15);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                } else {
                    // is a capture
                    float s = data.square_size * 0.3;
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x + s, base_y);
                    nvgLineTo(dc, base_x, base_y + s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x + data.square_size, base_y);
                    nvgLineTo(dc, base_x + data.square_size - s, base_y);
                    nvgLineTo(dc, base_x + data.square_size, base_y + s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x, base_y + data.square_size);
                    nvgLineTo(dc, base_x, base_y + data.square_size - s);
                    nvgLineTo(dc, base_x + s, base_y + data.square_size);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x + data.square_size, base_y + data.square_size);
                    nvgLineTo(dc, base_x + data.square_size - s, base_y + data.square_size);
                    nvgLineTo(dc, base_x + data.square_size, base_y + data.square_size - s);
                    nvgClosePath(dc);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                }
            }
        }
        nvgRestore(dc);
        nvgSave(dc);
        // render pinned piece
        if (data.mouse_pindx_x >= 0 && !data.passive_pin) {
            CHESS_piece pinned_piece;
            data.gi->get_cell(&data.g, data.mouse_pindx_x, data.mouse_pindx_y, &pinned_piece);
            nvgBeginPath(dc);
            nvgRect(dc, dd.x + data.mx - data.square_size / 2, dd.y + data.my - data.square_size / 2, data.square_size, data.square_size);
            int sprite_idx = pinned_piece.player * 6 - 6 + pinned_piece.type - 1;
            NVGpaint sprite_paint = nvgImagePattern(dc, dd.x + data.mx - data.square_size / 2, dd.y + data.my - data.square_size / 2, data.square_size, data.square_size, 0, data.sprites[sprite_idx], 1);
            nvgFillPaint(dc, sprite_paint);
            nvgFill(dc);
        }
        nvgRestore(dc);

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        if (strcmp(methods->game_name, "Chess") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0) {
            return ERR_OK;
        }
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods chess_fem{
    .frontend_name = "chess",
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
