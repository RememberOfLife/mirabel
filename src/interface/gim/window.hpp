#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include "nanovg.h"
#include "imgui.h"

#include "interface/interface.hpp"

class GraphicalImmediateMode : public Interface {
  private:

    SDL_Window* sdl_window;
    SDL_GLContext sdl_glcontext;
    ImGuiIO* imgui_io;
    ImGuiViewport* imgui_viewport;
    NVGcontext* nanovg_ctx;

  public:

    GraphicalImmediateMode();

    ~GraphicalImmediateMode();

    bool mainloop();
};
