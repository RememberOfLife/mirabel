#include <cstdint>
#include <unordered_map>
#include <vector>


#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/chess.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "games/game_catalogue.hpp"
#include "games/chess.hpp"
#include "meta_gui/meta_gui.hpp"

#include "frontends/chess.hpp"

namespace Frontends {

    void Chess::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+s && my >= y && my <= y+s);
    }

    Chess::Chess():
        the_game(NULL),
        the_game_int(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
        // load sprites from res folder
        for (int i = 0; i < 12; i++) {
            sprites[i] = -1;
        }
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_KING-1] = nvgCreateImage(dc, "../res/games/chess/pwk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_QUEEN-1] = nvgCreateImage(dc, "../res/games/chess/pwq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_ROOK-1] = nvgCreateImage(dc, "../res/games/chess/pwr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_BISHOP-1] = nvgCreateImage(dc, "../res/games/chess/pwb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_KNIGHT-1] = nvgCreateImage(dc, "../res/games/chess/pwn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_WHITE*6-6+CHESS_PIECE_TYPE_PAWN-1] = nvgCreateImage(dc, "../res/games/chess/pwp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_KING-1] = nvgCreateImage(dc, "../res/games/chess/pbk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_QUEEN-1] = nvgCreateImage(dc, "../res/games/chess/pbq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_ROOK-1] = nvgCreateImage(dc, "../res/games/chess/pbr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_BISHOP-1] = nvgCreateImage(dc, "../res/games/chess/pbb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_KNIGHT-1] = nvgCreateImage(dc, "../res/games/chess/pbn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[CHESS_PLAYER_BLACK*6-6+CHESS_PIECE_TYPE_PAWN-1] = nvgCreateImage(dc, "../res/games/chess/pbp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        for (int i = 0; i < 12; i++) {
            if (sprites[i] < 0) {
                MetaGui::logf("#E chess: sprite loading failure #%d\n", i);
                //TODO any backup?
            }
        }
        // setup buttons for drag and drop
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                board_buttons[y][x] = sbtn{
                    static_cast<float>(x)*(square_size),
                    (7*square_size)-static_cast<float>(y)*(square_size),
                    square_size, false
                };
            }
        }
    }

    Chess::~Chess()
    {
        for (int i = 0; i < 12; i++) {
            nvgDeleteImage(dc, sprites[i]);
        }
    }

    void Chess::set_game(game* new_game)
    {
        the_game = new_game;
        the_game_int = the_game ? (chess_internal_methods*)the_game->methods->internal_methods : NULL;
    }

    void Chess::process_event(SDL_Event event)
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                mx = event.motion.x - x_px;
                my = event.motion.y - y_px;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    bool new_pickup = false;
                    // is proper left mouse button down event
                    int mX = event.button.x - x_px;
                    int mY = event.button.y - y_px;
                    mX -= w_px/2-(8*square_size)/2;
                    mY -= h_px/2-(8*square_size)/2;
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONDOWN) { // on down event, pickup piece that is hovered, if any
                                CHESS_piece sp;
                                the_game_int->get_cell(the_game, x, y, &sp);
                                if (passive_pin && board_buttons[y][x].hovered) {
                                    //TODO when placing a piece back onto itself to reset it, this will try to make that move
                                    // if a pinned piece is set, instead MOVE it to the mousedown location
                                    uint64_t target_move = (mouse_pindx_x<<12)|(mouse_pindx_y<<8)|(x<<4)|(y);
                                    the_game->methods->get_concrete_moves(the_game, pbuf, &move_cnt, moves);
                                    for (int i = 0; i < move_cnt; i++) {
                                        if (moves[i] == target_move) {
                                            Control::main_client->inbox.push(Control::f_event_game_move(target_move));
                                            break;
                                        }
                                    }
                                    // do not set new pickup here, so that the pin gets cleared automatically
                                } else if (board_buttons[y][x].hovered && sp.type != CHESS_PIECE_TYPE_NONE && sp.player == pbuf) {
                                    // new pickup
                                    new_pickup = true;
                                    mouse_pindx_x = x;
                                    mouse_pindx_y = y;
                                }
                            }
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                if (board_buttons[y][x].hovered) {
                                    // dropped piece onto a square
                                    if (x == mouse_pindx_x && y == mouse_pindx_y) {
                                        // dropped back onto the picked up piece
                                        passive_pin = true;
                                    } else {
                                        // dropped onto another square
                                        //TODO this code is getting hideous
                                        uint64_t target_move = (mouse_pindx_x<<12)|(mouse_pindx_y<<8)|(x<<4)|(y);
                                        the_game->methods->get_concrete_moves(the_game, pbuf, &move_cnt, moves);
                                        for (int i = 0; i < move_cnt; i++) {
                                            if (moves[i] == target_move) {
                                                Control::main_client->inbox.push(Control::f_event_game_move(target_move));
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (event.type == SDL_MOUSEBUTTONDOWN && !new_pickup) {
                        passive_pin = false;
                        mouse_pindx_x = -1;
                        mouse_pindx_y = -1;
                    }
                    // reset pin idx after processing of a click
                    if (event.type == SDL_MOUSEBUTTONUP && !passive_pin) {
                        mouse_pindx_x = -1;
                        mouse_pindx_y = -1;
                    }
                }
            } break;
        }
    }

    void Chess::update()
    {
        if (!the_game) {
            return;
        }
        the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            return;
        }
        // set button hovered
        int mX = mx;
        int mY = my;
        mX -= w_px/2-(8*square_size)/2;
        mY -= h_px/2-(8*square_size)/2;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                board_buttons[y][x].x = static_cast<float>(x)*(square_size);
                board_buttons[y][x].y = (7*square_size)-static_cast<float>(y)*(square_size);
                board_buttons[y][x].s = square_size;
                board_buttons[y][x].update(mX, mY);
            }
        }
        //TODO should really cache these instead of loading them new every frame
        move_map.clear();
        the_game->methods->get_concrete_moves(the_game, pbuf, &move_cnt, moves);
        for (int i = 0; i < move_cnt; i++) {
            uint8_t m_from = (moves[i] >> 8) & 0xFF;
            if (move_map.find(m_from) == move_map.end()) {
                move_map.emplace(m_from, std::vector<uint8_t>{});
            }
            uint8_t m_to = moves[i] & 0xFF;
            move_map.at(m_from).push_back(m_to);
        }
    }

    void Chess::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2-(8*square_size)/2, h_px/2-(8*square_size)/2);
        // colored board border for current/winning player
        float border_size = square_size*0.1;
        nvgBeginPath(dc);
        nvgStrokeWidth(dc, border_size);
        nvgRect(dc, -border_size, -border_size, 8*square_size+2*border_size, 8*square_size+2*border_size);
        if (!the_game) {
            nvgStrokeColor(dc, nvgRGB(128, 128, 128));
        } else {
            CHESS_PLAYER color_player = CHESS_PLAYER_NONE;
            the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
            if (pbuf_c == 0) {
                the_game->methods->get_results(the_game, &pbuf_c, &pbuf);
            }
            color_player = (CHESS_PLAYER)pbuf;
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
            the_game->methods->players_to_move(the_game, &pbuf_c, &pbuf);
        }
        nvgStroke(dc);
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int iy = 7-y;
                float base_x = x * square_size;
                float base_y = y * square_size;
                // draw base square
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, square_size, square_size);
                nvgFillColor(dc, ((x + y) % 2 == 0) ? nvgRGB(240, 217, 181) : nvgRGB(161, 119, 67));
                nvgFill(dc);
                // draw rank and file
                static char char_buf[2] = "-";
                float text_padding = square_size * 0.05;
                nvgFontSize(dc, square_size * 0.17);
                nvgFontFace(dc, "ff");
                nvgFillColor(dc, ((x + y) % 2 == 1) ? nvgRGB(240, 217, 181) : nvgRGB(161, 119, 67)); // flip color from base square
                if (x == 7) {
                    char_buf[0] = '1'+iy;
                    nvgBeginPath(dc);
                    nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
                    nvgText(dc, base_x + square_size - text_padding, base_y + text_padding, char_buf, NULL);
                }
                if (iy == 0) {
                    char_buf[0] = 'a'+x;
                    nvgBeginPath(dc);
                    nvgTextAlign(dc, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
                    nvgText(dc, base_x + text_padding, base_y + square_size - text_padding, char_buf, NULL);
                }
                if (!the_game) {
                    continue;
                }
                CHESS_piece piece_in_square;
                the_game_int->get_cell(the_game, x, iy, &piece_in_square);
                if (piece_in_square.player == CHESS_PLAYER_NONE) {
                    continue;
                }
                float sprite_alpha = 1;
                if (x == mouse_pindx_x && iy == mouse_pindx_y) {
                    sprite_alpha = 0.5;
                    // render green underlay under the picked up piece
                    nvgBeginPath(dc);
                    nvgRect(dc, base_x, base_y, square_size, square_size);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                }
                // render the piece sprites
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, square_size, square_size);
                int sprite_idx = piece_in_square.player*6-6+piece_in_square.type-1;
                NVGpaint sprite_paint = nvgImagePattern(dc, base_x, base_y, square_size, square_size, 0, sprites[sprite_idx], sprite_alpha);
                nvgFillPaint(dc, sprite_paint);
                nvgFill(dc);
            }
        }
        // render possible moves, if pinned piece exists
        uint8_t m_from = (mouse_pindx_x<<4)|(mouse_pindx_y);
        if (mouse_pindx_x >= 0 && move_map.find(m_from) != move_map.end()) {
            std::vector<uint8_t> moves = move_map.at(m_from);
            for (int i = 0; i < moves.size(); i++) {
                int ix = (moves[i] >> 4) & 0x0F;
                int iy = (moves[i] & 0x0F);
                float base_x = ix * square_size;
                float base_y = (7-iy) * square_size;
                CHESS_piece sp;
                the_game_int->get_cell(the_game, ix, iy, &sp);
                if (sp.type == CHESS_PIECE_TYPE_NONE) {
                    // is a move to empty square
                    nvgBeginPath(dc);
                    nvgCircle(dc, base_x+square_size/2, base_y+square_size/2, square_size*0.15);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                } else {
                    // is a capture
                    float s = square_size*0.3;
                    nvgBeginPath(dc);
                    nvgMoveTo(dc, base_x, base_y);
                    nvgLineTo(dc, base_x+s, base_y);
                    nvgLineTo(dc, base_x, base_y+s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x+square_size, base_y);
                    nvgLineTo(dc, base_x+square_size-s, base_y);
                    nvgLineTo(dc, base_x+square_size, base_y+s);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x, base_y+square_size);
                    nvgLineTo(dc, base_x, base_y+square_size-s);
                    nvgLineTo(dc, base_x+s, base_y+square_size);
                    nvgClosePath(dc);
                    nvgMoveTo(dc, base_x+square_size, base_y+square_size);
                    nvgLineTo(dc, base_x+square_size-s, base_y+square_size);
                    nvgLineTo(dc, base_x+square_size, base_y+square_size-s);
                    nvgClosePath(dc);
                    nvgFillColor(dc, nvgRGBA(56, 173, 105, 128));
                    nvgFill(dc);
                }
            }
        }
        nvgRestore(dc);
        // render pinned piece
        if (mouse_pindx_x >= 0 && !passive_pin) {
            CHESS_piece pinned_piece;
            the_game_int->get_cell(the_game, mouse_pindx_x, mouse_pindx_y, &pinned_piece);
            nvgBeginPath(dc);
            nvgRect(dc, mx-square_size/2, my-square_size/2, square_size, square_size);
            int sprite_idx = pinned_piece.player*6-6+pinned_piece.type-1;
            NVGpaint sprite_paint = nvgImagePattern(dc, mx-square_size/2, my-square_size/2, square_size, square_size, 0, sprites[sprite_idx], 1);
            nvgFillPaint(dc, sprite_paint);
            nvgFill(dc);
        }
    }

    void Chess::draw_options()
    {
        ImGui::SliderFloat("square size", &square_size, 40, 175, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    Chess_FEW::Chess_FEW():
        FrontendWrap("Chess")
    {}

    Chess_FEW::~Chess_FEW()
    {}
    
    bool Chess_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return (dynamic_cast<Games::Chess*>(base_game_variant) != nullptr);
    }
    
    Frontend* Chess_FEW::new_frontend()
    {
        return new Chess();
    }

    void Chess_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
