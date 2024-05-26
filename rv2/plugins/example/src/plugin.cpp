#include <stdint.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "nanovg.h"
#include "nanovg_gl.h"

#include "mirabel/mirabel.h"

#include "mirabel/plugin.h"

int inited = 0;

NVGcontext* ctx = NULL;

void init()
{
    while (inited == 0) {
        inited = SDL_GetTicks();
    }
    // initialize
#ifdef __EMSCRIPTEN__
    ctx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#else
    ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
}

void draw()
{
    if (inited == 0) {
        init();
    }
    if (ctx == NULL) {
        return;
    }
    // regular drawing
    nvgBeginFrame(ctx, 100, 100, 2);
    nvgBeginPath(ctx);
    nvgRect(ctx, 20, 20, 20, 20);
    int g = (get_monotonic_time_ms() / 10) % 255;
    nvgFillColor(ctx, nvgRGB(g, g, g));
    nvgFill(ctx);
    nvgEndFrame(ctx);
}
