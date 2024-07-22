#include <cstdint>
#include <cstdio>

#include "nanovg.h"

#include "mirabel/application.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_empty_frontend()
{
    // test nanovg
    nvgBeginFrame(nanovg_ctx, fedd.fbw, fedd.fbh, 1); //TODO correct device pixel ratio
    nvgSave(nanovg_ctx);

    nvgBeginPath(nanovg_ctx);
    nvgRect(nanovg_ctx, fedd.fex, fedd.fey, fedd.few, fedd.feh);
    nvgFillColor(nanovg_ctx, nvgRGB(114, 140, 153));
    nvgFill(nanovg_ctx);

    float circ_x = fedd.fex + fedd.few / 2;
    float circ_y = fedd.fey + fedd.feh / 2;
    float circ_r = fmin(fedd.few, fedd.feh) / 20;
    float circ_ro = circ_r * 2;

    nvgBeginPath(nanovg_ctx);
    nvgMoveTo(nanovg_ctx, circ_x, circ_y - circ_ro);
    nvgLineTo(nanovg_ctx, fedd.fex, fedd.fey);
    nvgLineTo(nanovg_ctx, circ_x - circ_ro, circ_y);
    // nvgMoveTo(nanovg_ctx, circ_x - circ_ro, circ_y);
    nvgLineTo(nanovg_ctx, fedd.fex, fedd.fey + fedd.feh);
    nvgLineTo(nanovg_ctx, circ_x, circ_y + circ_ro);
    // nvgMoveTo(nanovg_ctx, circ_x, circ_y + circ_ro);
    nvgLineTo(nanovg_ctx, fedd.fex + fedd.few, fedd.fey + fedd.feh);
    nvgLineTo(nanovg_ctx, circ_x + circ_ro, circ_y);
    // nvgMoveTo(nanovg_ctx, circ_x + circ_ro, circ_y);
    nvgLineTo(nanovg_ctx, fedd.fex + fedd.few, fedd.fey);
    // nvgLineTo(nanovg_ctx, circ_x, circ_y - circ_ro);
    nvgClosePath(nanovg_ctx);
    nvgFillColor(nanovg_ctx, nvgRGB(87, 122, 140));
    nvgFill(nanovg_ctx);

    nvgBeginPath(nanovg_ctx);
    nvgCircle(nanovg_ctx, circ_x, circ_y, circ_r);
    nvgStrokeWidth(nanovg_ctx, 10);
    nvgStrokeColor(nanovg_ctx, nvgRGB(0, 0, 0));
    nvgStroke(nanovg_ctx);

#ifndef __EMSCRIPTEN__
    char vstr[64];
    sprintf(vstr, "mirabel v%u.%u.%u", app_version.major, app_version.minor, app_version.patch);
    nvgFontSize(nanovg_ctx, 20);
    nvgFontFace(nanovg_ctx, "nanovg_bold");
    nvgTextAlign(nanovg_ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
    nvgFillColor(nanovg_ctx, nvgRGB(210, 210, 210));
    nvgText(nanovg_ctx, fedd.fex + fedd.few - 15, fedd.fey + fedd.feh - 15, vstr, NULL);
#endif

    nvgRestore(nanovg_ctx);
    nvgEndFrame(nanovg_ctx);
}
