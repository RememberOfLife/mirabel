#pragma once

#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "surena/util/semver.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "control/timeout_crash.hpp"
#include "engines/engine_manager.hpp"
#include "frontends/frontend_catalogue.hpp"
#include "network/network_client.hpp"

namespace Control {

    extern const semver client_version;

    class Client {

        public:

            TimeoutCrash t_tc; // client owns this
            TimeoutCrash::timeout_info tc_info;

            Network::NetworkClient* t_network = NULL;
            f_event_queue* network_send_queue = NULL;
            // offline server likely somewhere here

            SDL_Window* sdl_window;
            SDL_GLContext sdl_glcontext;
            ImGuiIO* imgui_io;
            NVGcontext* nanovg_ctx;

            f_event_queue inbox;

            game* the_game = NULL;
            uint64_t game_step = 1;
            //TODO game_timectl
            //TODO game_history

            Frontends::Frontend* frontend = NULL;
            Engines::EngineManager* engine_mgr;

            float dpi_scale;

            Client();
            ~Client();
            void loop();

    };

    extern Client* main_client;

}
