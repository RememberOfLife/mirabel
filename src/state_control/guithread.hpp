#pragma once

#include <thread>

#include "imgui.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "surena_game.hpp"

#include "frontends/frontend.hpp"
#include "state_control/event_queue.hpp"

namespace StateControl {

    class GuiThread {

        private:

            SDL_Window* sdl_window;
            SDL_GLContext sdl_glcontext;
            ImGuiIO* imgui_io;

        public:

            std::thread running_thread;
            event_queue inbox;

            surena::PerfectInformationGame* game;
            Frontends::Frontend* ctx;

            GuiThread();
            ~GuiThread();
            void loop();

            void start();
            void join();

    };

}
