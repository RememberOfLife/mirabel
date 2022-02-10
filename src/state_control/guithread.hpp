#pragma once

#include <thread>

#include "SDL.h"
#include "SDL_opengl.h"
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"
#include "state_control/event_queue.hpp"

namespace StateControl {

    class GuiThread {

        private:

            SDL_Window* sdl_window;
            SDL_GLContext sdl_glcontext;
            ImGuiIO* imgui_io;
            NVGcontext* nanovg_ctx;

        public:

            std::thread running_thread;
            event_queue inbox;

            surena::Game* game;
            Frontends::Frontend* frontend;
            surena::Engine* engine;

            GuiThread();
            ~GuiThread();
            void loop();

            void start();
            void join();

    };

}
