#include <cstdint>

#include <SDL2/SDL.h>
#include "nanovg.h"
#include "imgui.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "frontends/empty_frontend.hpp"

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
