#include <chrono>
#include <cstdint>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "surena_game.hpp"
#include "surena_tictactoe.hpp"

#include "games/default_ctx.hpp"
#include "games/tictactoe.hpp"
#include "meta_gui/meta_gui.hpp"
#include "state_control/drawing_context_app.hpp"
#include "state_control/event.hpp"
#include "state_control/event_queue.hpp"

#include "state_control/guithread.hpp"

namespace StateControl {

    GuiThread::GuiThread():
        game(NULL),
        ctx(NULL)
    {
        // setup SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            fprintf(stderr, "sdl init error: %s\n", SDL_GetError());
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

        // init default context
        ctx = new Games::DefaultCtx();
    }

    GuiThread::~GuiThread()
    {
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
        bool show_demo_window = false;
        float w_px = imgui_io->DisplaySize.x;
        float h_px = imgui_io->DisplaySize.y;

        int frame_work_ns = 0;
        const int frame_budget_ns = (1000 * 1000 * 1000)/60;
        bool quit = false;
        while (!quit) {
            // sleep for the dead time that would be wasted by rendering
            // reduces input lag considerably by waiting up to the last possible moment to gather input events before action+rendering
            std::this_thread::sleep_for(std::chrono::nanoseconds(frame_budget_ns-frame_work_ns));
            // start measuring event + action and render time
            std::chrono::steady_clock::time_point frame_time_start = std::chrono::steady_clock::now();

            for (event e = inbox.pop(); e.type != 0; e = inbox.pop()) {
                // process event e
                // e.g. gamestate updates, load other ctx or game, etc..
                switch (e.type) {
                    case EVENT_TYPE_LOADGAME: {
                        MetaGui::log("loading ttt game\n");
                        delete game;
                        game = new surena::TicTacToe();
                        ctx->game = game;
                    } break;
                    case EVENT_TYPE_LOADCTX: {
                        MetaGui::log("loading ttt ctx\n");
                        delete ctx;
                        ctx = new Games::TicTacToe();
                        ctx->game = game;
                    } break;
                    case EVENT_TYPE_MOVE: {
                        MetaGui::log("performing ttt move\n");
                        game->apply_move(reinterpret_cast<uint64_t>(e.data1));
                        if (game->player_to_move() == 0) {
                            MetaGui::logf("#S ttt game done: winner is %d\n", game->get_result());
                        }
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
                    quit = true;
                    break;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl_window)) {
                    quit = true;
                    break;
                }
                // global window shortcuts
                if (!imgui_io->WantCaptureKeyboard && event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_F3) {
                        MetaGui::show_stats_overlay = !MetaGui::show_stats_overlay;
                    }
                    if (event.key.keysym.sym == SDLK_F4) {
                        MetaGui::show_logs_window = !MetaGui::show_logs_window;
                    }
                    if (event.key.keysym.sym == SDLK_F5) {
                        show_demo_window = !show_demo_window;
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
                ctx->process_event(event);
            }

            // start the dear imgui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // show imgui windows
            if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
            if (MetaGui::show_main_menu_bar) MetaGui::main_menu_bar(&MetaGui::show_main_menu_bar);
            if (MetaGui::show_stats_overlay) MetaGui::stats_overlay(&MetaGui::show_stats_overlay);
            if (MetaGui::show_logs_window) MetaGui::logs_window(&MetaGui::show_logs_window);
            if (MetaGui::show_gamestate_config_window) MetaGui::gamestate_config_window(&MetaGui::show_gamestate_config_window);
            if (MetaGui::show_guistate_config_window) MetaGui::guistate_config_window(&MetaGui::show_guistate_config_window);
            if (MetaGui::show_engine_window) MetaGui::engine_window(&MetaGui::show_engine_window);

            //TODO put this in the sdl resize event, make a resize function on the context app
            w_px = imgui_io->DisplaySize.x;
            h_px = imgui_io->DisplaySize.y;
            ctx->w_px = w_px;
            ctx->h_px = h_px;
            // rendering
            glViewport(0, 0, (int)w_px, (int)h_px);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, (GLdouble)w_px, (GLdouble)h_px, 0.0, -1, 1);

            ctx->update();
            ctx->render();
            
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(sdl_window);

            std::chrono::steady_clock::time_point frame_time_stop = std::chrono::steady_clock::now();
            frame_work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time_stop-frame_time_start).count();
        }
    }

}
