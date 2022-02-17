#include <cstdint>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/games/chess.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "games/chess.hpp"
#include "meta_gui/meta_gui.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/chess.hpp"

namespace Frontends {

    void Chess::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+s && my >= y && my <= y+s);
    }

    Chess::Chess():
        game(NULL),
        engine(NULL)
    {
        dc = StateControl::main_ctrl->t_gui.nanovg_ctx;
        // load sprites from res folder
        for (int i = 0; i < 12; i++) {
            sprites[i] = -1;
        }
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_KING-1] = nvgCreateImage(dc, "../res/games/chess/pwk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_QUEEN-1] = nvgCreateImage(dc, "../res/games/chess/pwq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_ROOK-1] = nvgCreateImage(dc, "../res/games/chess/pwr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_BISHOP-1] = nvgCreateImage(dc, "../res/games/chess/pwb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_KNIGHT-1] = nvgCreateImage(dc, "../res/games/chess/pwn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_WHITE*6-6+surena::Chess::PIECE_TYPE_PAWN-1] = nvgCreateImage(dc, "../res/games/chess/pwp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_KING-1] = nvgCreateImage(dc, "../res/games/chess/pbk.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_QUEEN-1] = nvgCreateImage(dc, "../res/games/chess/pbq.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_ROOK-1] = nvgCreateImage(dc, "../res/games/chess/pbr.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_BISHOP-1] = nvgCreateImage(dc, "../res/games/chess/pbb.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_KNIGHT-1] = nvgCreateImage(dc, "../res/games/chess/pbn.png", NVG_IMAGE_GENERATE_MIPMAPS);
        sprites[surena::Chess::PLAYER_BLACK*6-6+surena::Chess::PIECE_TYPE_PAWN-1] = nvgCreateImage(dc, "../res/games/chess/pbp.png", NVG_IMAGE_GENERATE_MIPMAPS);
        for (int i = 0; i < 12; i++) {
            if (sprites[i] < 0) {
                MetaGui::logf("#E chess: sprite loading failure #%d\n", i);
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

    void Chess::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::Chess*>(new_game);
    }

    void Chess::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    void Chess::process_event(SDL_Event event)
    {
        if (!game || game->player_to_move() == 0) {
            // if no game, or game is done, don't process anything
            return;
        }
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                mx = event.motion.x;
                my = event.motion.y;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (event.type == SDL_MOUSEBUTTONDOWN) {
                        passive_pin = false;
                        mouse_pindx_x = -1;
                        mouse_pindx_y = -1;
                    }
                    // is proper left mouse button down event
                    int mX = event.button.x;
                    int mY = event.button.y;
                    mX -= w_px/2-(8*square_size)/2;
                    mY -= h_px/2-(8*square_size)/2;
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONDOWN) { // on down event, pickup piece that is hovered, if any
                                if (board_buttons[y][x].hovered && game->get_cell(x, y).type != surena::Chess::PIECE_TYPE_NONE) {
                                    mouse_pindx_x = x;
                                    mouse_pindx_y = y;
                                }
                            }
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                if (board_buttons[y][x].hovered) {
                                    // dropped piece onto a square
                                    if (x == mouse_pindx_x && y == mouse_pindx_y) {
                                        // dropped back onto the picked up piece
                                        passive_pin = true; //TODO if a passively pinned piece is dropped onto itself AGAIN, then reset the passive pin
                                    } else {
                                        // dropped onto another square
                                        //TODO
                                        StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_move_event(StateControl::EVENT_TYPE_GAME_MOVE, (mouse_pindx_x<<12)|(mouse_pindx_y<<8)|(x<<4)|(y)));
                                    }
                                }
                            }
                        }
                    }
                }
                // reset pin idx after processing of a click
                if (event.type == SDL_MOUSEBUTTONUP && !passive_pin) {
                    mouse_pindx_x = -1;
                    mouse_pindx_y = -1;
                }
            } break;
        }
    }

    void Chess::update()
    {
        if (!game || game->player_to_move() == 0) {
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
    }

    void Chess::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2-(8*square_size)/2, h_px/2-(8*square_size)/2);
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
                if (!game) {
                    continue;
                }
                surena::Chess::piece piece_in_square = game->get_cell(x, iy);
                if (piece_in_square.player == surena::Chess::PLAYER_NONE) {
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
        nvgRestore(dc);
        // render pinned piece
        if (mouse_pindx_x >= 0 && !passive_pin) {
            surena::Chess::piece pinned_piece = game->get_cell(mouse_pindx_x, mouse_pindx_y);
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
        ImGui::SliderFloat("square size", &square_size, 40, 125, "%.3f", ImGuiSliderFlags_AlwaysClamp);
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
