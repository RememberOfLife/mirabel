#pragma once

#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nanovg_gl.h"
#include "imgui.h"
#include "rosalia/semver.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/frontend.h"
#include "control/plugins.hpp"
#include "control/timeout_crash.hpp"
// #include "engines/engine_manager.hpp" //TODO REENABLE engine
#include "network/network_client.hpp"

namespace Control {

    extern const semver client_version;

    class Client {

      public:

        TimeoutCrash t_tc; // client owns this
        TimeoutCrash::timeout_info tc_info;

        Network::NetworkClient* t_network = NULL;
        event_queue* network_send_queue = NULL;
        uint32_t lobby_id = EVENT_LOBBY_NONE;
        // offline server likely somewhere here

        SDL_Window* sdl_window;
        SDL_GLContext sdl_glcontext;
        ImGuiIO* imgui_io;
        NVGcontext* nanovg_ctx;

        event_queue inbox;

        game* the_game = NULL;
        uint32_t game_sync = 1; //TODO use and use in server
        uint64_t game_step = 1;
        //TODO game_timectl
        //TODO game_history

        PluginManager plugin_mgr;

        frontend_display_data dd;
        frontend* the_frontend;
        frontend* empty_fe;
        // Engines::EngineManager* engine_mgr; //TODO REENABLE engine

        float dpi_scale;

        Client();
        ~Client();
        void loop();
    };

    extern Client* main_client;

} // namespace Control
