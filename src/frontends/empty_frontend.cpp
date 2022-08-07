#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/game.h"

#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"
#include "games/game_catalogue.hpp"

#include "frontends/empty_frontend.hpp"
#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    EmptyFrontend::EmptyFrontend()
    {
        dc = Control::main_client->nanovg_ctx;
        sprintf(vstr, "mirabel v%u.%u.%u", Control::client_version.major, Control::client_version.minor, Control::client_version.patch);
    }

    EmptyFrontend::~EmptyFrontend()
    {}

    void EmptyFrontend::set_game(game* new_game)
    {}

    void EmptyFrontend::process_event(SDL_Event event)
    {}

    void EmptyFrontend::update()
    {}

    void EmptyFrontend::render()
    {
        nvgSave(dc);
        nvgBeginPath(dc);
        nvgRect(dc, -10, -10, w_px+20, h_px+20);
        nvgFillColor(dc, nvgRGB(114, 140, 153));
        nvgFill(dc);

        nvgFontSize(dc, 20);
        nvgFontFace(dc, "ff");
        nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgText(dc, w_px - 15, h_px - 15, vstr, NULL);

        nvgRestore(dc);
    }

    void EmptyFrontend::draw_options()
    {
        //TODO make an option for the background color
        ImGui::TextDisabled("<no options>");
    }

    EmptyFrontend_FEW::EmptyFrontend_FEW():
        FrontendWrap("<empty>")
    {}

    EmptyFrontend_FEW::~EmptyFrontend_FEW()
    {}
    
    bool EmptyFrontend_FEW::game_methods_compatible(const game_methods* methods)
    {
        return false; // empty frontend will never be listed explicitly, but can always be used as universal unloader by loading it instead
    }
    
    Frontend* EmptyFrontend_FEW::new_frontend()
    {
        return new EmptyFrontend();
    }

    void EmptyFrontend_FEW::draw_options()
    {
        ImGui::TextDisabled("<no options>");
    }

}

namespace {

    error_code opts_create(void** options_struct)
    {
        return ERR_OK;
    }

    error_code opts_display(void* options_struct)
    {
        return ERR_OK;
    }

    error_code opts_destroy(void* options_struct)
    {
        return ERR_OK;
    }

    const char* get_last_error(frontend* self)
    {
        //TODO
        return NULL;
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        self->data1 = malloc(64);
        sprintf((char*)self->data1, "mirabel v%u.%u.%u", Control::client_version.major, Control::client_version.minor, Control::client_version.patch);
        return ERR_OK;
    }

    error_code destroy(frontend* self)
    {
        free(self->data1);
        return ERR_OK;
    }

    error_code runtime_opts_display(frontend* self)
    {
        return ERR_OK;
    }

    error_code process_event(frontend* self, f_event_any event)
    {
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        return ERR_OK;
    }

    error_code update(frontend* self, player_id view)
    {
        return ERR_OK;
    }

    //TODO both background and render would usually want to overdraw a little so borders get drawn correctly, this is not ok with new param setup

    error_code background(frontend* self, float x, float y, float w, float h)
    {
        NVGcontext* dc = Control::main_client->nanovg_ctx;

        //TODO how to get the dc in a proper way?
        nvgSave(dc);

        nvgBeginPath(dc);
        nvgRect(dc, x-10, y-10, w+20, h+20);
        nvgFillColor(dc, nvgRGB(114, 140, 153));
        nvgFill(dc);

        nvgRestore(dc);

        return ERR_OK;
    }

    error_code render(frontend* self, player_id view, float x, float y, float w, float h)
    {
        NVGcontext* dc = Control::main_client->nanovg_ctx;

        //TODO how to get the dc in a proper way?
        nvgSave(dc);

        nvgBeginPath(dc);
        nvgRect(dc, x-10, y-10, w+20, h+20);
        nvgFillColor(dc, nvgRGB(114, 140, 153));
        nvgFill(dc);

        nvgFontSize(dc, 20);
        nvgFontFace(dc, "ff");
        nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgText(dc, x + w - 15, y + h - 15, (char*)self->data1, NULL);

        nvgRestore(dc);
        
        return ERR_OK;
    }

    error_code is_game_compatible(game* compat_game)
    {
        return ERR_INVALID_INPUT;
    }

}

const frontend_methods empty_fem{
    .frontend_name = "<empty>", // violates naming convention, but this one is special
    .version = semver{0, 1, 0},
    .features = frontend_feature_flags{
        .options = false,
        .global_background = true,
    },

    .internal_methods = NULL,

    .opts_create = opts_create,
    .opts_display = opts_display,
    .opts_destroy = opts_destroy,

    .get_last_error = get_last_error,

    .create = create,
    .destroy = destroy,

    .runtime_opts_display = runtime_opts_display,

    .process_event = process_event,
    .process_input = process_input,
    .update = update,

    .background = background,
    .render = render,

    .is_game_compatible = is_game_compatible,    

};
