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
    }

    Chess::~Chess()
    {}

    void Chess::set_game(surena::Game* new_game)
    {
        game = dynamic_cast<surena::Chess*>(new_game);
    }

    void Chess::set_engine(surena::Engine* new_engine)
    {
        engine = new_engine;
    }

    /*
    TODO
    drag and drop behaviour:

    static piece drag_piece;

    on mouse down on a piece set it as drag_piece, also reset passively pinned piece
    while drag_piece != NULL
        draw possible moves for this piece, with small markers, also highlight the origin square of the piece
        keep sprite of piece floating at the mouse location, transparently faded version stays on origin square
    on mouse up while drag piece is set:
        if the target square is the origin square of the piece, set it as passively pinned,
            i.e. show its possible moves without requiring any mouse interaction
        if the target square is not the origin:
            enqueue move if target square is a legal move
            reset drag piece otherwise

    possibly cache available moves by piece, maybe even just a map, as to not require move gen every frame
    */

    void Chess::process_event(SDL_Event event)
    {
        
    }

    void Chess::update()
    {
        
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
                surena::Chess::piece piece_in_square = game->get_cell(x, (7-y));
                if (piece_in_square.player == surena::Chess::PLAYER_NONE) {
                    continue;
                }
                // render the piece sprites
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, square_size, square_size);
                int sprite_idx = piece_in_square.player*6-6+piece_in_square.type-1;
                NVGpaint sprite_paint = nvgImagePattern(dc, base_x, base_y, square_size, square_size, 0, sprites[sprite_idx], 1);
                nvgFillPaint(dc, sprite_paint);
                nvgFill(dc);
            }
        }
        nvgRestore(dc);
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
