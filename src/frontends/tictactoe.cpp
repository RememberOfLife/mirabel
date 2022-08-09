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
#include "games/game_catalogue.hpp"

#include "frontends/tictactoe.hpp"
#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    void TicTacToe::sbtn::update(float mx, float my) {
        hovered = (mx >= x && mx <= x+w && my >= y && my <= y+h);
    }

    TicTacToe::TicTacToe():
        the_game(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x] = sbtn{
                    static_cast<float>(x)*(button_size+padding),
                    (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding),
                    button_size, button_size, false, false
                };
            }
        }
    }

    TicTacToe::~TicTacToe()
    {}

    void TicTacToe::set_game(game* new_game)
    {
        the_game = new_game;
        the_game_int = the_game ? (tictactoe_internal_methods*)the_game->methods->internal_methods : NULL;
    }

    void TicTacToe::process_event(SDL_Event event)
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
                    // is proper left mouse button down event, find where it clicked and if applicable push the appropriate event
                    int mX = event.button.x - x_px;
                    int mY = event.button.y - y_px;
                    mX -= w_px/2-(3*button_size+2*padding)/2;
                    mY -= h_px/2-(3*button_size+2*padding)/2;
                    for (int x = 0; x < 3; x++) {
                        for (int y = 0; y < 3; y++) {
                            board_buttons[y][x].update(mX, mY);
                            if (event.type == SDL_MOUSEBUTTONUP) {
                                player_id cell_player;
                                the_game_int->get_cell(the_game, x, y, &cell_player);
                                if (board_buttons[y][x].hovered && board_buttons[y][x].mousedown && cell_player == 0) {
                                    uint64_t move_code = x | (y<<2);
                                    f_event_any es;
                                    f_event_create_game_move(&es, move_code);
                                    f_event_queue_push(&Control::main_client->inbox, &es);
                                }
                                board_buttons[y][x].mousedown = false;
                            }
                            board_buttons[y][x].mousedown |= (board_buttons[y][x].hovered && event.type == SDL_MOUSEBUTTONDOWN);
                        }
                    }
                }
            } break;
        }
    }

    void TicTacToe::update()
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
        mX -= w_px/2-(3*button_size+2*padding)/2;
        mY -= h_px/2-(3*button_size+2*padding)/2;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                board_buttons[y][x].x = static_cast<float>(x)*(button_size+padding);
                board_buttons[y][x].y = (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding);
                board_buttons[y][x].w = button_size;
                board_buttons[y][x].h = button_size;
                board_buttons[y][x].update(mX, mY);
            }
        }
    }

    void TicTacToe::render()
    {
        nvgSave(dc);
        nvgStrokeWidth(dc, button_size*0.175);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(201, 144, 73));
        nvgFill(dc);
        nvgTranslate(dc, w_px/2-(3*button_size+2*padding)/2, h_px/2-(3*button_size+2*padding)/2);
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                float base_x = static_cast<float>(x)*(button_size+padding);
                float base_y = (2*button_size+2*padding)-static_cast<float>(y)*(button_size+padding);
                nvgBeginPath(dc);
                nvgRect(dc, base_x, base_y, button_size, button_size);
                if (!the_game || pbuf_c == 0) {
                    nvgFillColor(dc, nvgRGB(161, 119, 67));
                } else {
                    nvgFillColor(dc, nvgRGB(240, 217, 181));
                }
                nvgFill(dc);
                if (!the_game) {
                    continue;
                }
                player_id player_in_cell;
                the_game_int->get_cell(the_game, x, y, &player_in_cell);
                if (player_in_cell == 1) {
                    // X
                    nvgBeginPath(dc);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgMoveTo(dc, base_x+button_size*0.175, base_y+button_size*0.175);
                    nvgLineTo(dc, base_x+button_size*0.825, base_y+button_size*0.825);
                    nvgMoveTo(dc, base_x+button_size*0.175, base_y+button_size*0.825);
                    nvgLineTo(dc, base_x+button_size*0.825, base_y+button_size*0.175);
                    nvgStroke(dc);
                } else if (player_in_cell == 2) {
                    // O
                    nvgBeginPath(dc);
                    nvgStrokeColor(dc, nvgRGB(25, 25, 25));
                    nvgCircle(dc, base_x+button_size/2, base_y+button_size/2, button_size*0.3);
                    nvgStroke(dc);
                } else if (board_buttons[y][x].hovered && pbuf > PLAYER_NONE) {
                    nvgBeginPath(dc);
                    nvgFillColor(dc, nvgRGB(220, 197, 161));
                    nvgRect(dc, board_buttons[y][x].x+button_size*0.05, board_buttons[y][x].y+button_size*0.05, board_buttons[y][x].w-button_size*0.1, board_buttons[y][x].h-button_size*0.1);
                    nvgFill(dc);
                }
                //TODO
                /*if (engine && engine->player_to_move() != 0 && engine->get_best_move() == ((y<<2)|x)) {
                    nvgBeginPath(dc);
                    nvgFillColor(dc, nvgRGB(125, 187, 248));
                    nvgCircle(dc, board_buttons[y][x].x+button_size/2, board_buttons[y][x].y+button_size/2, button_size*0.15);
                    nvgFill(dc);
                }*/
            }
        }
        nvgRestore(dc);
    }

    void TicTacToe::draw_options()
    {
        ImGui::SliderFloat("button size", &button_size, 20, 400, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("padding", &padding, 0, 100, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    TicTacToe_FEW::TicTacToe_FEW():
        FrontendWrap("TicTacToe")
    {}

    TicTacToe_FEW::~TicTacToe_FEW()
    {}
    
    bool TicTacToe_FEW::game_methods_compatible(const game_methods* methods)
    {
        return (strcmp(methods->game_name, "TicTacToe") == 0 && strcmp(methods->variant_name, "Standard") == 0 && strcmp(methods->impl_name, "surena_default") == 0);
    }
    
    Frontend* TicTacToe_FEW::new_frontend()
    {
        return new TicTacToe();
    }

    void TicTacToe_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}

namespace {

    const char* get_last_error(frontend* self)
    {
        //TODO
        return NULL;
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        //TODO
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        //TODO
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        //TODO
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        //TODO
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        //TODO
        return ERR_OK;
    }

    error_code update(frontend* self, player_id view)
    {
        //TODO
        return ERR_OK;
    }
    
    error_code render(frontend* self, player_id view, float x, float y, float w, float h)
    {
        //TODO
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
    .version = semver{0, 1, 0},
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
