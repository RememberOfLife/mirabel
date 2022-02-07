#pragma once

#include <cstdint>

#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace StateControl {

    enum EVENT_TYPE {
        EVENT_TYPE_NULL = 0,
        EVENT_TYPE_EXIT,
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_MOVE,
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
    };

    struct game_event {
        uint32_t type;
        surena::PerfectInformationGame* game;
    };

    struct move_event {
        uint32_t type;
        uint64_t code;
    };
    
    struct frontend_event {
        uint32_t type;
        Frontends::Frontend* frontend;
    };

    union event {
        uint32_t type;
        game_event game;
        move_event move;
        frontend_event frontend;

        event(uint32_t type);
        event(const event& e);
        static event create_game_event(uint32_t type, surena::PerfectInformationGame* game);
        static event create_move_event(uint32_t type, uint64_t code);
        static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
    };

}
