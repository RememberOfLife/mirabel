#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event_queue.h"
#include "control/event.h"
#include "games/game_catalogue.hpp"

#include "frontends/fallback_text.hpp"

namespace Frontends {

    //TODO using this frontend has lots of double frees and other memory corruptions, and also crashed the sdl poll event somehow

    FallbackText::FallbackText():
        the_game(NULL)
    {
        dc = Control::main_client->nanovg_ctx;
    }

    FallbackText::~FallbackText()
    {
        free(pbuf);
        pbuf = NULL;
        pbuf_c = 0;
        free(rbuf);
        rbuf = NULL;
        rbuf_c = 0;
        free(opts_str);
        opts_str = NULL;
        free(state_str);
        state_str = NULL;
        free(print_str);
        print_str = NULL;
    }

    void FallbackText::set_game(game* new_game)
    {
        the_game = new_game;
        if (the_game) {
            pbuf = (player_id*)malloc(the_game->sizer.max_players_to_move);
            rbuf = (player_id*)malloc(the_game->sizer.max_results);
            if (the_game->methods->features.options) {
                opts_str = (char*)malloc(the_game->sizer.options_str);
            }
            state_str = (char*)malloc(the_game->sizer.state_str);
            if (the_game->methods->features.print) {
                print_str = (char*)malloc(the_game->sizer.print_str);
            }
        } else {
            free(pbuf);
            pbuf = NULL;
            pbuf_c = 0;
            free(rbuf);
            rbuf = NULL;
            rbuf_c = 0;
            free(opts_str);
            opts_str = NULL;
            free(state_str);
            state_str = NULL;
            free(print_str);
            print_str = NULL;
        }
    }

    void FallbackText::process_event(SDL_Event event)
    {}

    void FallbackText::update()
    {
        if (!the_game) {
            return;
        }
        if (the_game_step == Control::main_client->game_step) {
            return;
        }
        the_game_step = Control::main_client->game_step;
        size_t throwaway_size;
        the_game->methods->players_to_move(the_game, &pbuf_c, pbuf);
        if (pbuf_c == 0) {
            the_game->methods->get_results(the_game, &rbuf_c, rbuf);
        } else {
            rbuf_c = 0;
        }
        if (the_game->methods->features.options) {
            the_game->methods->export_options_str(the_game, &throwaway_size, opts_str);
        }
        the_game->methods->export_state(the_game, &throwaway_size, state_str);
        if (the_game->methods->features.id) {
            the_game->methods->id(the_game, &the_id);
        }
        if (the_game->methods->features.print) {
            the_game->methods->debug_print(the_game, &throwaway_size, print_str);
        }
    }

    void FallbackText::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgFill(dc);
        
        nvgFontSize(dc, 24);
        nvgFontFace(dc, "mf");
        
        nvgFillColor(dc, nvgRGB(25, 25, 25));
        nvgTextAlign(dc, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);

        if (the_game) {
            //TODO prefix all the strings by a title what they are to their left, aligned top right

            if (the_game->methods->features.options) {
                nvgBeginPath(dc);
                nvgText(dc, 50, 50, opts_str, NULL);
            }
            nvgBeginPath(dc);
            nvgText(dc, 50, 100, state_str, NULL);
            if (the_game->methods->features.id) {
                char id_str[32];
                sprintf(id_str, "ID#%016lx", the_id);
                nvgBeginPath(dc);
                nvgText(dc, 50, 150, id_str, NULL);
            }
            if (the_game->methods->features.print) {
                nvgBeginPath(dc);
                //TODO print in multiple lines for newlines
                float lineoffset = 0;
                char* strstart = print_str;
                char* strend = strchr(strstart, '\n');
                nvgText(dc, 50, 200 + lineoffset, strstart, strend);
                // while (strend != NULL) {
                //     printf("%p %p\n", strstart, strend);
                //     lineoffset += 25;
                //     strstart = strend + 1;
                //     char* strend = strchr(strstart, '\n');
                // }
            }
        
            //TODO render all available moves and make them clickable
        
        }

        nvgRestore(dc);
    }

    void FallbackText::draw_options()
    {
        //TODO font size options
    }

    FallbackText_FEW::FallbackText_FEW():
        FrontendWrap("FallbackText")
    {}

    FallbackText_FEW::~FallbackText_FEW()
    {}
    
    bool FallbackText_FEW::base_game_variant_compatible(Games::BaseGameVariant* base_game_variant)
    {
        return true;
    }
    
    Frontend* FallbackText_FEW::new_frontend()
    {
        return new FallbackText();
    }

    void FallbackText_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}
