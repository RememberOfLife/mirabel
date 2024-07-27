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
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "rosalia/semver.h"
#include "rosalia/timestamp.h"

#include "mirabel/log.h"

#include "interface/gim/window.h"

#include "interface/gim/window.hpp"

bool graphical_immediate_mode_interface::create()
{
    sdl_window = NULL;
    sdl_glcontext = NULL;
    imgui_io = NULL;
    imgui_viewport = NULL;
    nanovg_ctx = NULL;

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
    imgui_io->ConfigDockingAlwaysTabBar = true;
    imgui_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // makes hotkeys uncomfortable because imgui grabs attention for nav
    // setup imgui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    imgui_io->IniFilename = NULL; //TODO reenable, but for now turn it off to test sane defaults..

    // dpi scaling
    float dpi_scale = 1;
    float dpi;
    if (!SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(sdl_window), &dpi, NULL, NULL)) {
        dpi_scale = dpi / 96; // 96 is the default dpi on windows
        if (dpi_scale < 1 || dpi_scale > 4) { // sanity check, would underscale < 1 on normal display (looks blurry)
            dpi_scale = 1;
        }
        mirabel_slogf(LOGS_LESS, "imgui dpi scale adjusted: %.2f", dpi_scale);
    }
    imgui_io->FontGlobalScale = dpi_scale;
    //TODO load font with approriate size instead of scaling it!

    // setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_glcontext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    imgui_viewport = ImGui::GetMainViewport();

#ifndef __EMSCRIPTEN__
    //TODO embed basic imgui font in web build and give it a loadable location via the resource manager
    //TODO load imgui fonts: docs/FONTS.md
    // ImFontConfig font_config;
    // font_config.RasterizerDensity = 2;
    // font_config.OversampleH = 2;
    // font_config.OversampleV = 2;
    // use as &font_config last arg for loading
    //TODO can use the returned fonts for font push/pop, save them somewhere
    float font_size_normal = 22; // or 20, but no less
    fonts.imgui_reg = imgui_io->Fonts->AddFontFromFileTTF("../res/fonts/opensans/OpenSans-Regular.ttf", font_size_normal);
    fonts.imgui_bold = imgui_io->Fonts->AddFontFromFileTTF("../res/fonts/opensans/OpenSans-Bold.ttf", font_size_normal);
    fonts.imgui_italic = imgui_io->Fonts->AddFontFromFileTTF("../res/fonts/opensans/OpenSans-Italic.ttf", font_size_normal);
    fonts.imgui_mono = imgui_io->Fonts->AddFontFromFileTTF("../res/fonts/liberation-mono/LiberationMono-Regular.ttf", font_size_normal * 0.9); //TODO mono font is too big compared to opensans
#else
    ImFont* imgui_default = imgui_io->Fonts->Fonts[0];
    fonts.imgui_reg = imgui_default;
    fonts.imgui_bold = imgui_default;
    fonts.imgui_italic = imgui_default;
    fonts.imgui_mono = imgui_default;
#endif

    //TODO this doesnt work on web, we have to render everything to a separate framebuffer and resolve it manually
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

#ifndef __EMSCRIPTEN__
    //TODO same embedded basic font as the imgui basic one
    //TODO load nanovg fonts
    //TODO also returns the font handle but not really needed truly
    nvgCreateFont(nanovg_ctx, "nanovg_bold", "../res/fonts/opensans/OpenSans-Bold.ttf");
#endif

    show_imgui_demo = false;
    show_about_info = false;
    fullscreen = false;

    fedd.fbw = -1;
    fedd.fbh = -1;
    //TODO guard this so it is NULL when not properly initialized
    glGenFramebuffers(1, &fedd.frontend_fbo);
    glGenTextures(1, &fedd.frontend_tex);
    glGenRenderbuffers(1, &fedd.frontend_rbo);

    metagui_new_workspace();

    return false;
}

void graphical_immediate_mode_interface::destroy()
{
    //TODO gracefully destroy frontend fbo + rbo + tex

    if (nanovg_ctx != NULL) {
#ifdef __EMSCRIPTEN__
        nvgDeleteGLES2(nanovg_ctx);
#else
        nvgDeleteGL3(nanovg_ctx);
#endif
    }

    if (imgui_viewport != NULL) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }
    if (imgui_io != NULL) {
        ImGui::DestroyContext();
    }

    if (sdl_glcontext != NULL) {
        SDL_GL_DeleteContext(sdl_glcontext);
    }
    if (sdl_window != NULL) {
        SDL_DestroyWindow(sdl_window);
    }
    SDL_Quit();

    delete this;
}

bool graphical_immediate_mode_interface::update_and_render()
{
    bool quit = false;

    static bool ctrl_left = false;
    static bool ctrl_right = false;

#ifndef __EMSCRIPTEN__
    {
        static int frame_work_ns = 0;
        static uint64_t frame_ts_start = 0;
        static bool init_frame_time = false;
        if (init_frame_time) {
            const uint64_t frame_ts_stop = timestamp_get_ns64();
            frame_work_ns = frame_ts_stop - frame_ts_start;
        }
        init_frame_time = true;
        const int frame_budget_ns = (1000 * 1000 * 1000) / 60;
        if (frame_work_ns < frame_budget_ns) {
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = frame_budget_ns - frame_work_ns;
            while (clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem) == EINTR) {
                req = rem;
            }
        }
        frame_ts_start = timestamp_get_ns64();
    }
#endif

    // static uint64_t ms_tick = timestamp_get_ms64(); //TODO move to correct place(s)

    //TODO this should be a function
    //TODO process client internal event queue (or is that on another thread?)

    //TODO this hsould be a function
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

        // global window shortcuts
        if (event.type == SDL_KEYDOWN) {
            if ((ctrl_left || ctrl_right) && event.key.keysym.sym == SDLK_n) {
                metagui_new_workspace();
            }
            if (event.key.keysym.sym == SDLK_F1) {
                //TODO toggle hud, i.e. skipp all imgui rendering and the frontend assumes the entire framebuffer size
            }
            if (event.key.keysym.sym == SDLK_F3) {
                //TODO toggle stats
            }
            if (event.key.keysym.sym == SDLK_F4) {
                show_log = !show_log;
            }
            if (event.key.keysym.sym == SDLK_F5) {
                show_imgui_demo = !show_imgui_demo;
            }
            if (event.key.keysym.sym == SDLK_F11) {
#ifndef __EMSCRIPTEN__
                fullscreen = !fullscreen;
                // borderless fullscreen
                SDL_SetWindowFullscreen(sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#else
                mirabel_slogf(LOGS_NORM, "fullscreen currently unsupported in web");
#endif
            }
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

    //TODO put this in the sdl resize event, make a resize function on the context app?
    fedd.fbw = imgui_viewport->Size.x;
    fedd.fbh = imgui_viewport->Size.y;
    glViewport(0, 0, (int)fedd.fbw, (int)fedd.fbh);

    {
        // draw background to refresh behind imgui windows
        nvgBeginFrame(nanovg_ctx, fedd.fbw, fedd.fbh, 1); //TODO correct device pixel ratio
        nvgSave(nanovg_ctx);

        nvgBeginPath(nanovg_ctx);
        nvgRect(nanovg_ctx, imgui_viewport->WorkPos.x, imgui_viewport->WorkPos.y, imgui_viewport->WorkSize.x, imgui_viewport->WorkSize.y);
        nvgFillColor(nanovg_ctx, nvgRGB(0, 0, 0));
        nvgFill(nanovg_ctx);

        nvgRestore(nanovg_ctx);
        nvgEndFrame(nanovg_ctx);
    }

    if (show_imgui_demo) {
        ImGui::ShowDemoWindow();
        fedd.fex = imgui_viewport->WorkPos.x;
        fedd.fey = imgui_viewport->WorkPos.y;
        fedd.few = imgui_viewport->WorkSize.x;
        fedd.feh = imgui_viewport->WorkSize.y;
        metagui_empty_frontend();
    } else {
        metagui_main_menu_bar();

        ImGui::DockSpaceOverViewport();
        for (size_t workspace_idx = 0; workspace_idx < workspaces.size(); workspace_idx++) {
            metagui_workspace_window(workspace_idx);
        }
        //TODO this all goes into the workspace frontend breadcrumb
        // update ticks for frontend or no?
        // update frontend
        // render frontend

        metagui_log();
        metagui_about_info();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(sdl_window);

    return quit;
}

/////
// methods wrapper

#ifdef __cplusplus
extern "C" {
#endif

static const char* app_interface_get_last_error_mi(app_interface* self)
{
    return NULL;
}

static error_code app_interface_create_mi(app_interface* self)
{
    self->data = new graphical_immediate_mode_interface{};
    if (((graphical_immediate_mode_interface*)self->data)->create()) {
        return APP_INTERFACE_ERR_NOK;
    }
    return APP_INTERFACE_ERR_OK;
}

static void app_interface_destroy_mi(app_interface* self)
{
    ((graphical_immediate_mode_interface*)self->data)->destroy();
}

static bool app_interface_mainloop_mi(app_interface* self)
{
    return ((graphical_immediate_mode_interface*)self->data)->update_and_render();
}

static void app_interface_log_mi(app_interface* self, LOGS status, const char* str, const char* str_end)
{
    graphical_immediate_mode_interface* self_interface = (graphical_immediate_mode_interface*)self->data;
    graphical_immediate_mode_interface::log_entry new_log_entry{
        .time = timestamp_get_ms64(),
        .status = status,
        .msg = NULL,
    };
    if (str_end == NULL) {
        new_log_entry.msg = (str == NULL ? NULL : strdup(str));
    } else if (str != NULL) {
        size_t msg_len = str_end - str;
        new_log_entry.msg = (char*)malloc(msg_len + 1);
        memcpy(new_log_entry.msg, str, msg_len);
        new_log_entry.msg[msg_len] = '\0';
    }
    self_interface->stored_logs.push_back(new_log_entry);
}

static const char* app_interface_user_file_path_prompt_mi(app_interface* self, const char* suggested_save_name)
{
    //TODO
    return NULL;
}

const app_interface_methods gim_app_interface_methods = (app_interface_methods){
    .name = "gim",
    .version = (semver){
        .major = 0,
        .minor = 0,
        .patch = 0,
    },
    .get_last_error = app_interface_get_last_error_mi,
    .create = app_interface_create_mi,
    .destroy = app_interface_destroy_mi,
    .mainloop = app_interface_mainloop_mi,
    .log = app_interface_log_mi,
    .user_file_path_prompt = app_interface_user_file_path_prompt_mi,
};

#ifdef __cplusplus
}
#endif
