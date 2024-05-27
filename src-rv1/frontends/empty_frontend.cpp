#include <cstdint>

#include <SDL.h>
#include "nanovg.h"
#include "imgui.h"

#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "mirabel/game.h"
#include "control/client.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace {

    struct data_repr {
        NVGcontext* dc;
        frontend_display_data* dd;
        char vstr[64];
    };

    data_repr& _get_repr(frontend* self)
    {
        return *((data_repr*)(self->data1));
    }

    error_code create(frontend* self, frontend_display_data* display_data, void* options_struct)
    {
        self->data1 = malloc(sizeof(data_repr));
        data_repr& data = _get_repr(self);
        data = (data_repr){
            .dc = Control::main_client->nanovg_ctx,
            .dd = display_data,
        };
        sprintf(data.vstr, "mirabel v%u.%u.%u", Control::client_version.major, Control::client_version.minor, Control::client_version.patch);
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

    error_code process_event(frontend* self, event_any event)
    {
        event_destroy(&event);
        return ERR_OK;
    }

    error_code process_input(frontend* self, SDL_Event event)
    {
        return ERR_OK;
    }

    error_code update(frontend* self)
    {
        return ERR_OK;
    }

    error_code render(frontend* self)
    {
        data_repr& data = _get_repr(self);
        NVGcontext* dc = data.dc;
        frontend_display_data& dd = *data.dd;

        nvgBeginFrame(dc, dd.fbw, dd.fbh, 2); //TODO use proper devicePixelRatio

        nvgSave(dc);

        nvgBeginPath(dc);
        nvgRect(dc, dd.x - 10, dd.y - 10, dd.w + 20, dd.h + 20);
        nvgFillColor(dc, nvgRGB(114, 140, 153));
        nvgFill(dc);

        nvgFontSize(dc, 20);
        nvgFontFace(dc, "ff");
        nvgTextAlign(dc, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
        nvgFillColor(dc, nvgRGB(210, 210, 210));
        nvgText(dc, dd.x + dd.w - 15, dd.y + dd.h - 15, data.vstr, NULL);

        nvgRestore(dc);

        nvgEndFrame(dc);

        return ERR_OK;
    }

    error_code is_game_compatible(const game_methods* methods)
    {
        return ERR_INVALID_INPUT;
    }

} // namespace

const frontend_methods empty_fem{
    .frontend_name = "<empty>", // violates naming convention, but this one is special
    .version = semver{1, 0, 0},
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
