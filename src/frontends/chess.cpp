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
#include "prototype_util/direct_draw.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "frontends/chess.hpp"

namespace Frontends {

    Chess::Chess():
        game(NULL),
        engine(NULL)
    {}

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

    void Chess::render(NVGcontext* ctx)
    {
        nvgSave(ctx);
        nvgBeginPath(ctx);
        nvgRect(ctx, -10, -10, w_px+20, h_px+20);
        nvgFillColor(ctx, nvgRGB(201, 144, 73));
        nvgFill(ctx);
        nvgTranslate(ctx, w_px/2-(8*square_size)/2, h_px/2-(8*square_size)/2);
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                float base_x = x * square_size;
                float base_y = y * square_size;
                // draw base square
                nvgBeginPath(ctx);
                nvgRect(ctx, base_x, base_y, square_size, square_size);
                nvgFillColor(ctx, ((x + y) % 2 == 0) ? nvgRGB(240, 217, 181) : nvgRGB(161, 119, 67));
                nvgFill(ctx);
                if (!game) {
                    continue;
                }
                surena::Chess::piece piece_in_square = game->get_cell(x, (7-y));
                if (piece_in_square.player == surena::Chess::PLAYER_NONE) {
                    continue;
                }
                //TODO this should render the piece sprites instead of just text bubbles
                nvgBeginPath(ctx);
                nvgFillColor(ctx, nvgRGBA(0, 0, 0, 40));
                nvgCircle(ctx, base_x+square_size/2, base_y+square_size/2, square_size*0.4);
                nvgFill(ctx);
                nvgBeginPath(ctx);
                nvgFontFace(ctx, "ff");
                nvgFontSize(ctx, square_size*0.7);
                nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                base_y += square_size/4;
                switch (piece_in_square.player) {
                    case surena::Chess::PLAYER_WHITE: {
                        nvgFillColor(ctx, nvgRGB(236, 236, 236));
                    } break;
                    case surena::Chess::PLAYER_BLACK: {
                        nvgFillColor(ctx, nvgRGB(25, 25, 25));
                    } break;
                    default: {
                        assert(0);
                    } break;
                }
                switch (piece_in_square.type) {
                    case surena::Chess::PIECE_TYPE_KING: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "K", NULL);
                    } break;
                    case surena::Chess::PIECE_TYPE_QUEEN: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "Q", NULL);
                    } break;
                    case surena::Chess::PIECE_TYPE_ROOK: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "R", NULL);
                    } break;
                    case surena::Chess::PIECE_TYPE_BISHOP: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "B", NULL);
                    } break;
                    case surena::Chess::PIECE_TYPE_KNIGHT: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "N", NULL);
                    } break;
                    case surena::Chess::PIECE_TYPE_PAWN: {
                        nvgText(ctx, base_x+square_size/2, base_y+square_size/2, "P", NULL);
                    } break;
                    default: {
                        assert(0);
                    } break;
                }
                nvgFill(ctx);
            }
        }
        nvgRestore(ctx);
    }

    void Chess::draw_options()
    {
        ImGui::SliderFloat("button size", &square_size, 40, 125, "%.3f", ImGuiSliderFlags_AlwaysClamp);
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
