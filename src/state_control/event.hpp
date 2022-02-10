#pragma once

#include <cstdint>

#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace StateControl {

    enum EVENT_TYPE {
        EVENT_TYPE_NULL = 0,
        EVENT_TYPE_EXIT,
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_MOVE,
        EVENT_TYPE_GAME_INTERNAL_UPDATE,
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
        EVENT_TYPE_ENGINE_LOAD,
        EVENT_TYPE_ENGINE_UNLOAD,
    };

    struct game_event {
        surena::Game* game;
    };

    struct move_event {
        uint64_t code;
    };

    struct internal_update_event {
        uint64_t code;
    };
    
    struct frontend_event {
        Frontends::Frontend* frontend;
    };

    struct engine_event {
        surena::Engine* engine;
    };

    struct event {
        uint32_t type;
        union {
            game_event game;
            move_event move;
            internal_update_event internal_update;
            frontend_event frontend;
            engine_event engine;
        };
        event(uint32_t type);
        event(const event& e);
        static event create_game_event(uint32_t type, surena::Game* game);
        static event create_move_event(uint32_t type, uint64_t code);
        static event create_internal_update_event(uint32_t type, uint64_t code);
        static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
        static event create_engine_event(uint32_t type, surena::Engine* frontend);
    };

}
