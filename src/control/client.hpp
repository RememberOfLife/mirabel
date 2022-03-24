#pragma once

#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/game.hpp"

#include "control/event_queue.hpp"
#include "control/timeout_crash.hpp"
#include "frontends/frontend_catalogue.hpp"
#include "network/network_client.hpp"

namespace Control {

    class Client {

        public:

            TimeoutCrash t_tc; // client owns this
            TimeoutCrash::timeout_info tc_info;

            Network::NetworkClient* t_network = NULL;
            event_queue* network_send_queue = NULL;
            // offline server likely somewhere here

            SDL_Window* sdl_window;
            SDL_GLContext sdl_glcontext;
            ImGuiIO* imgui_io;
            NVGcontext* nanovg_ctx;

            event_queue inbox;

            surena::Game* game = NULL;
            Frontends::Frontend* frontend = NULL;
            surena::Engine* engine = NULL;

            Client();
            ~Client();
            void loop();

    };

    extern Client* main_client;

}
