#include <ctime>
#include <errno.h>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
#include "nanovg_gl.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "rosalia/semver.h"
#include "rosalia/timestamp.h"

#include "mirabel/log.h"

#include "interface/gim/window.h"

#include "interface/gim/window.hpp"

void global_dockspace(float* x, float* y, float* w, float* h)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    dockspace_flags |= ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_None;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    host_window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    host_window_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("global_dockspace_window", NULL, host_window_flags);
    ImGui::PopStyleVar(3);
    ImGuiID dockspace_id = ImGui::GetID("global_dockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGuiDockNode* dn = ImGui::DockBuilderGetNode(dockspace_id);
    *x = dn->CentralNode->Pos.x;
    *y = dn->CentralNode->Pos.y;
    *w = dn->CentralNode->Size.x;
    *h = dn->CentralNode->Size.y;
    ImGui::End();
}

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
    if (nanovg_ctx == NULL) {
        mirabel_slogf(LOGS_FATAL, "nanovg context creation failed\n");
        exit(1);
    }

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

    //TODO move out to display data header
    //TODO and cul for usefulness..
    float x_px = imgui_viewport->WorkPos.x;
    float y_px = imgui_viewport->WorkPos.y;
    float w_px = imgui_viewport->WorkSize.x;
    float h_px = imgui_viewport->WorkSize.y;
    float fx_px = x_px;
    float fy_px = y_px;
    float fw_px = w_px;
    float fh_px = h_px;
    float fbw = imgui_viewport->Size.x;
    float fbh = imgui_viewport->Size.y;
    float fex = fx_px - x_px;
    float fey = fy_px - y_px;
    float few = fw_px;
    float feh = fh_px;
    static bool ctrl_left = false;
    static bool ctrl_right = false;

#ifndef __EMSCRIPTEN__
    static int frame_work_ns = 0;
    const int frame_budget_ns = (1000 * 1000 * 1000) / 60;
    if (frame_work_ns < frame_budget_ns) {
        struct timespec req, rem;
        req.tv_sec = 0;
        req.tv_nsec = frame_budget_ns - frame_work_ns;
        while (clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem) == EINTR) {
            req = rem;
        }
    }
    const uint64_t frame_ts_start = timestamp_get_ns64();
#endif

    // static uint64_t ms_tick = timestamp_get_ms64(); //TODO move to correct place(s)

    //TODO process client internal event queue (or is that on another thread?)

    // work through interface events: clicks, key presses, gui commands structs for updating interface elems
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // pass event through imgui
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            quit = true;
            break;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl_window)) {
            quit = true;
            break;
        }
        //TODO confirm exit modal hotkey has precedence
        // imgui wants mouse: skip mouse events
        if (imgui_io->WantCaptureMouse && (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL)) {
            continue;
        }
        // imgui wants keyboard: skip keyboard events
        if (imgui_io->WantCaptureKeyboard && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {
            continue;
        }
        //TODO our own input event handling from here

        //TODO use these to actually set the viewport and so on, so we dont have to check it every frame from imgui
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            // printf("sdl size changed to %i / %i\n", event.window.data1, event.window.data2);
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            // printf("sdl resized to %i / %i\n", event.window.data1, event.window.data2);
        }

        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_LCTRL) {
                ctrl_left = true;
            } else if (event.key.keysym.sym == SDLK_RCTRL) {
                ctrl_right = true;
            }
        }
        if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_LCTRL) {
                ctrl_left = false;
            } else if (event.key.keysym.sym == SDLK_RCTRL) {
                ctrl_right = false;
            }
        }
    }

    // start the dear imgui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    global_dockspace(&fx_px, &fy_px, &fw_px, &fh_px);

    //TODO show imgui windows
    ImGui::ShowDemoWindow();

    //TODO put this in the sdl resize event, make a resize function on the context app
    // whole workspace under the menubar, use this for frontend background if wanted
    //TODO just need w and h of whole window, replace
    x_px = imgui_viewport->WorkPos.x;
    y_px = imgui_viewport->WorkPos.y;
    w_px = imgui_viewport->WorkSize.x;
    h_px = imgui_viewport->WorkSize.y;

    fbw = imgui_viewport->Size.x;
    fbh = imgui_viewport->Size.y;
    // frontend only gets the frontend dockspace
    fex = fx_px;
    fey = fy_px;
    few = fw_px;
    feh = fh_px;

    static int printed = 0;
    if (printed++ < 20) {
        // printf("%f %f %f %f // %f %f // %f %f %f %f\n", x_px, y_px, w_px, h_px, fbw, fbh, fex, fey, few, feh);
    }

    glViewport(0, 0, (int)fbw, (int)fbh);

    // test nanovg
    nvgBeginFrame(nanovg_ctx, fbw, fbh, 2);
    nvgSave(nanovg_ctx);
    nvgBeginPath(nanovg_ctx);
    nvgRect(nanovg_ctx, fex, fey, few, feh);
    nvgFillColor(nanovg_ctx, nvgRGB(114, 140, 153));
    nvgFill(nanovg_ctx);
    nvgBeginPath(nanovg_ctx);
    // nvgRect(nanovg_ctx, fex + few - 40, fey + feh - 40, 30, 30);
    nvgMoveTo(nanovg_ctx, 100, 100);
    nvgLineTo(nanovg_ctx, 200, 200);
    nvgStrokeWidth(nanovg_ctx, 10);
    nvgStrokeColor(nanovg_ctx, nvgRGB(0, 0, 0));
    nvgStroke(nanovg_ctx);
    nvgRestore(nanovg_ctx);
    nvgEndFrame(nanovg_ctx);

    //TODO update ticks for frontend or no?
    //TODO update frontend
    //TODO render frontend

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(sdl_window);

#ifndef __EMSCRIPTEN__
    const uint64_t frame_ts_stop = timestamp_get_ns64();
    frame_work_ns = frame_ts_stop - frame_ts_start;
#endif

    return quit;
}

/////
// methods wrapper

#ifdef __cplusplus
extern "C" {
#endif

static const char* get_last_error_cif(client_interface* self)
{
    return NULL;
}

static error_code create_cif(client_interface* self)
{
    self->data = new GraphicalImmediateMode();
    return CLIENT_INTERFACE_ERR_OK;
}

static error_code destroy_cif(client_interface* self)
{
    delete (GraphicalImmediateMode*)self->data;
    return CLIENT_INTERFACE_ERR_OK;
}

static bool mainloop_cif(client_interface* self)
{
    return ((GraphicalImmediateMode*)self->data)->mainloop();
}

static void log_cif(client_interface* self, LOGS status, const char* str, const char* str_end)
{
    //TODO
}

static const char* user_file_path_prompt_cif(client_interface* self, const char* suggested_save_name)
{
    //TODO
    return NULL;
}

const client_interface_methods gim_client_interface{
    .name = "gim",
    .version = (semver){
        .major = 0,
        .minor = 0,
        .patch = 0,
    },
    .get_last_error = get_last_error_cif,
    .create = create_cif,
    .destroy = destroy_cif,
    .mainloop = mainloop_cif,
    .log = log_cif,
    .user_file_path_prompt = user_file_path_prompt_cif,
};

#ifdef __cplusplus
}
#endif
