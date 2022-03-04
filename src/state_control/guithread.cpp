#include <chrono>
#include <cstdint>
#include <cstdio>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "surena/game.hpp"

#include "frontends/empty_frontend.hpp"
#include "frontends/frontend_catalogue.hpp"
#include "frontends/tictactoe.hpp"
#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"
#include "state_control/event.hpp"
#include "state_control/event_queue.hpp"

#include "state_control/guithread.hpp"

namespace StateControl {

    GuiThread::GuiThread():
        game(NULL),
        frontend(NULL),
        engine(NULL)
    {
        // setup SDL
        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
        {
            fprintf(stderr, "[FATAL] sdl init error: %s\n", SDL_GetError());
            exit(-1);
        }

        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        // create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,16);
        SDL_GL_SetSwapInterval(1); // vsync with 1, possibly set after window creation
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        sdl_window = SDL_CreateWindow("mirabel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        sdl_glcontext = SDL_GL_CreateContext(sdl_window);
        SDL_GL_MakeCurrent(sdl_window, sdl_glcontext);

        // setup imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imgui_io = &ImGui::GetIO(); (void)imgui_io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // enable keyboard controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // enable gamepad controls

        // setup imgui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // setup platform/renderer backends
        ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_glcontext);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // ImFontConfig font_config;
        // font_config.OversampleH = 4;
        // font_config.OversampleV = 4;
        // io.Fonts->AddFontFromFileTTF("../fonts/opensans/OpenSans-Regular.ttf", 20.0f, &font_config);

        glEnable(GL_MULTISAMPLE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        nanovg_ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
        int font_id0 = nvgCreateFont(nanovg_ctx, "df", "../res/fonts/opensans/OpenSans-Regular.ttf");
        if (font_id0 < 0) {
            fprintf(stderr, "[FATAL] nvg failed to load font 0\n");
            exit(-1);
        }
        int font_id1 = nvgCreateFont(nanovg_ctx, "ff", "../res/fonts/opensans/OpenSans-ExtraBold.ttf");
        if (font_id1 < 0) {
            fprintf(stderr, "[FATAL] nvg failed to load font 1\n");
            exit(-1);
        }

        // init default context
        frontend = new Frontends::EmptyFrontend();
    }

    GuiThread::~GuiThread()
    {
        delete engine;
        delete frontend;
        delete game;

        //TODO delete loaded font images

        nvgDeleteGL3(nanovg_ctx);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(sdl_glcontext);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
    }

    void GuiThread::loop()
    {
        //TODO cleanup statics here
        bool show_hud = true;
        bool fullscreen = false;
        bool show_demo_window = false;
        float w_px = imgui_io->DisplaySize.x;
        float h_px = imgui_io->DisplaySize.y;

        bool ctrl_left = false;
        bool ctrl_right = false;

        int frame_work_ns = 0;
        const int frame_budget_ns = (1000 * 1000 * 1000)/60;
        bool quit = false;
        bool try_quit = false;
        while (!quit) {
            // sleep for the dead time that would be wasted by rendering
            // reduces input lag considerably by waiting up to the last possible moment to gather input events before action+rendering
            std::this_thread::sleep_for(std::chrono::nanoseconds(frame_budget_ns-frame_work_ns));
            // start measuring event + action and render time
            std::chrono::steady_clock::time_point frame_time_start = std::chrono::steady_clock::now();

            for (event e = inbox.pop(); e.type != 0; e = inbox.pop()) {
                // process event e
                // e.g. game updates, load other ctx or game, etc..
                switch (e.type) {
                    case EVENT_TYPE_EXIT: {
                        try_quit = true;
                        break;
                    } break;
                    case EVENT_TYPE_GAME_LOAD: {
                        delete game;
                        game = e.game.game;
                        frontend->set_game(game);
                        if (engine) {
                            engine->set_gamestate(game->clone());
                        }
                    } break;
                    case EVENT_TYPE_GAME_UNLOAD: {
                        frontend->set_game(NULL);
                        if (engine) {
                            engine->set_gamestate(NULL);
                        }
                        delete game;
                        game = NULL;
                    } break;
                    case EVENT_TYPE_GAME_MOVE: {
                        game->apply_move(e.move.code);
                        if (engine) {
                            engine->apply_move(e.move.code);
                        }
                        if (game->player_to_move() == 0) {
                            MetaGui::logf("game done: winner is player %d\n", game->get_result());
                        }
                    } break;
                    case EVENT_TYPE_FRONTEND_LOAD: {
                        delete frontend;
                        frontend = e.frontend.frontend;
                        frontend->set_game(game);
                        frontend->set_engine(engine);
                        MetaGui::running_few_idx = MetaGui::selected_few_idx;
                    } break;
                    case EVENT_TYPE_FRONTEND_UNLOAD: {
                        delete frontend;
                        frontend = new Frontends::EmptyFrontend();
                        MetaGui::running_few_idx = 0;
                    } break;
                    case EVENT_TYPE_ENGINE_LOAD: {
                        delete engine;
                        engine = e.engine.engine;
                        engine->set_gamestate(game ? game->clone() : NULL);
                        frontend->set_engine(engine);
                    } break;
                    case EVENT_TYPE_ENGINE_UNLOAD: {
                        frontend->set_engine(NULL);
                        delete engine;
                        engine = NULL;
                    } break;
                    default: {
                        MetaGui::logf("#W guithread: received unknown event, type: %d\n", e.type);
                    } break;
                }
            }
            
            // work through interface events: clicks, key presses, gui commands structs for updating interface elems
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                // pass event through imgui
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT) {
                    try_quit = true;
                    break;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl_window)) {
                    try_quit = true;
                    break;
                }
                // if the confirm_exit_modal is up, pass CTRL+Q quit hotkey anyway
                if (MetaGui::show_confirm_exit_modal) {
                    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q && (ctrl_left || ctrl_right)) {
                        try_quit = true;
                        break;
                    }
                }
                // imgui wants mouse: skip mouse events
                if (imgui_io->WantCaptureMouse && (
                    event.type == SDL_MOUSEMOTION ||
                    event.type == SDL_MOUSEBUTTONDOWN ||
                    event.type == SDL_MOUSEBUTTONUP ||
                    event.type == SDL_MOUSEWHEEL)) {
                    continue;
                }
                // imgui wants keyboard: skip keyboard events
                if (imgui_io->WantCaptureKeyboard && (
                    event.type == SDL_KEYDOWN ||
                    event.type == SDL_KEYUP)) {
                    continue;
                }
                // global window shortcuts
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_F1) {
                        show_hud = !show_hud;
                    }
                    if (event.key.keysym.sym == SDLK_F3) {
                        MetaGui::show_stats_overlay = !MetaGui::show_stats_overlay;
                    }
                    if (event.key.keysym.sym == SDLK_F4) {
                        MetaGui::show_logs_window = !MetaGui::show_logs_window;
                    }
                    if (event.key.keysym.sym == SDLK_F5) {
                        show_demo_window = !show_demo_window;
                    }
                    if (event.key.keysym.sym == SDLK_F11) {
                        fullscreen = !fullscreen;
                        // borderless fullscreen
                        SDL_SetWindowFullscreen(sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    }
                    if (event.key.keysym.sym == SDLK_g && (ctrl_left || ctrl_right)) {
                        MetaGui::show_game_config_window = !MetaGui::show_game_config_window;
                    }
                    if (event.key.keysym.sym == SDLK_f && (ctrl_left || ctrl_right)) {
                        MetaGui::show_frontend_config_window = !MetaGui::show_frontend_config_window;
                    }
                    if (event.key.keysym.sym == SDLK_e && (ctrl_left || ctrl_right)) {
                        MetaGui::show_engine_window = !MetaGui::show_engine_window;
                    }
                    if (event.key.keysym.sym == SDLK_q && (ctrl_left || ctrl_right)) {
                        try_quit = true;
                        break;
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
                frontend->process_event(event);
            }
            // user is trying to quit
            if (try_quit) {
                if (!game) {
                    quit = true;
                    break;
                } else {
                    // game running, display quit modal, or quit if already shown
                    if (MetaGui::show_confirm_exit_modal) {
                        quit = true;
                        break;
                    } else {
                        MetaGui::show_confirm_exit_modal = true;
                    }
                }
                try_quit = false;
            }

            // start the dear imgui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // show imgui windows
            //TODO since all of these only show when the bool is set, it doesnt really need to be an argument, they can just check themselves
            if (show_hud) {
                if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
                if (MetaGui::show_confirm_exit_modal) MetaGui::confirm_exit_modal(&MetaGui::show_confirm_exit_modal);
                if (MetaGui::show_main_menu_bar) MetaGui::main_menu_bar(&MetaGui::show_main_menu_bar);
                if (MetaGui::show_stats_overlay) MetaGui::stats_overlay(&MetaGui::show_stats_overlay);
                if (MetaGui::show_logs_window) MetaGui::logs_window(&MetaGui::show_logs_window);
                if (MetaGui::show_game_config_window) MetaGui::game_config_window(&MetaGui::show_game_config_window);
                if (MetaGui::show_frontend_config_window) MetaGui::frontend_config_window(&MetaGui::show_frontend_config_window);
                if (MetaGui::show_engine_window) MetaGui::engine_window(&MetaGui::show_engine_window);
            }

            //TODO put this in the sdl resize event, make a resize function on the context app
            // also this should use workarea and not entire viewport, as it otherwise slip under the main menu bar
            w_px = imgui_io->DisplaySize.x;
            h_px = imgui_io->DisplaySize.y;
            frontend->w_px = w_px;
            frontend->h_px = h_px;
            // rendering
            glViewport(0, 0, (int)w_px, (int)h_px);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, (GLdouble)w_px, (GLdouble)h_px, 0.0, -1, 1);

            frontend->update();
            nvgBeginFrame(nanovg_ctx, w_px, h_px, 2); //TODO use proper devicePixelRatio
            frontend->render(); // todo render should take the player perspective (information view) from which to draw
            nvgEndFrame(nanovg_ctx);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(sdl_window);

            std::chrono::steady_clock::time_point frame_time_stop = std::chrono::steady_clock::now();
            frame_work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time_stop-frame_time_start).count();
        }
    }

}
