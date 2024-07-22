#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include "nanovg.h"
#include "imgui.h"

struct graphical_immediate_mode_interface {

    SDL_Window* sdl_window;
    SDL_GLContext sdl_glcontext;
    ImGuiIO* imgui_io;
    ImGuiViewport* imgui_viewport;
    NVGcontext* nanovg_ctx;

    bool show_imgui_demo;
    bool show_about_info;
    bool fullscreen;

    //TODO better place for this
    //TODO and cul for usefulness..
    struct {
        // bool complete; //TODO so we actually only use it, if it is GL complete
        GLuint frontend_fbo;
        GLuint frontend_tex;
        GLuint frontend_rbo;
        float fbw;
        float fbh;
        //TODO these should in reality all be removable
        float fex;
        float fey;
        float few;
        float feh;
    } fedd;

    static graphical_immediate_mode_interface* create();
    void destroy();

    bool update_and_render();

    // metagui
    void metagui_about_info();
    void metagui_empty_frontend();
    void metagui_global_dockspace(float* x, float* y, float* w, float* h);
    void metagui_main_menu_bar();
    void metagui_workspace_tabs();
};
