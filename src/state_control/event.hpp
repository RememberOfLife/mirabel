#pragma once

#include <cstdint>

namespace StateControl {

    //TODO type enum
    enum EVENT_TYPE {
        EVENT_TYPE_NULL = 0,
        EVENT_TYPE_EXIT,
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_MOVE,
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
    };

    struct event {
        uint32_t type;
        uint32_t code;
        void* data1;
        event(uint32_t type);
        event(uint32_t type, uint32_t code, void* data1);
        event(const event& e);
    };

}
