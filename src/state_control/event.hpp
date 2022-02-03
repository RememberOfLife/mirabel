#pragma once

#include <cstdint>

namespace StateControl {

    //TODO type enum
    enum EVENT_TYPE {
        EVENT_TYPE_NULL = 0,
        EVENT_TYPE_LOADGAME,
        EVENT_TYPE_LOADCTX,
        EVENT_TYPE_MOVE,
    };

    struct event {
        uint32_t type;
        uint32_t code;
        void* data1;
        event(uint32_t type, uint32_t code, void* data1);
        event(const event& e);
    };

}
