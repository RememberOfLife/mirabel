#include <GL/glew.h>
#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
#include "nanovg_gl.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "mirabel/log.h"

#include "interface/gim/window.hpp"

//HACK to get it working without proper log for now
#define mirabel_slogf(status, fmt, ...) printf(fmt, __VA_ARGS__)

GraphicalImmediateMode::GraphicalImmediateMode()
{
    const int initial_window_width = 1280;
    const int initial_window_height = 720;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        mirabel_slogf(LOGS_FATAL, "sdl init error: %s\n", SDL_GetError());
        exit(1);
    }

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); //TODO want adjustable (16 is just better) can't be 16 b/c then window creation fails on low end devices
    SDL_GL_SetSwapInterval(1); // vsync with 1, possibly set after window creation

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    sdl_window = SDL_CreateWindow("mirabel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, initial_window_width, initial_window_height, window_flags);
    if (sdl_window == NULL) {
        mirabel_slogf(LOGS_FATAL, "sdl window create error: %s\n", SDL_GetError());
        exit(1);
    }

    sdl_glcontext = SDL_GL_CreateContext(sdl_window);
    if (sdl_glcontext == NULL) {
        mirabel_slogf(LOGS_FATAL, "sdl gl context create error: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_GL_MakeCurrent(sdl_window, sdl_glcontext);

    // SDL_GL_SetSwapInterval(0); //TODO enable vsync?

    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        mirabel_slogf(LOGS_FATAL, "glew init error: %s\n", glewGetErrorString(glew_err));
        exit(1);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_io = &ImGui::GetIO();
    (void)imgui_io;
    imgui_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // enable gamepad controls
    // setup imgui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // dpi scaling
    float dpi_scale = 1;
    float dpi;
    if (!SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(sdl_window), &dpi, NULL, NULL)) {
        dpi_scale = dpi / 96; // 96 is the default dpi on windows
        if (dpi_scale < 1 || dpi_scale > 4) { // sanity check, would underscale < 1 on normal display (looks blurry)
            dpi_scale = 1;
        }
    }
    imgui_io->FontGlobalScale = dpi_scale;
    //TODO load font with approriate size instead of scaling it!

    // setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_glcontext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    imgui_viewport = ImGui::GetMainViewport();

    //TODO load imgui fonts: docs/FONTS.md

    // ImFontConfig font_config;
    // font_config.OversampleH = 4;
    // font_config.OversampleV = 4;
    // io.Fonts->AddFontFromFileTTF("../fonts/opensans/OpenSans-Regular.ttf", 20.0f, &font_config);

    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

#ifdef __EMSCRIPTEN__
    nanovg_ctx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#else
    nanovg_ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif

    //TODO load nanovg fonts
}

GraphicalImmediateMode::~GraphicalImmediateMode()
{
#ifdef __EMSCRIPTEN__
    nvgDeleteGLES2(nanovg_ctx);
#else
    nvgDeleteGL3(nanovg_ctx);
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(sdl_glcontext);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

bool GraphicalImmediateMode::mainloop()
{
    bool quit = false;

    // start the dear imgui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(sdl_window);

    return quit;
}
