#include <cstddef>

#include "state_control/event.hpp"

namespace StateControl {

    event::event(uint32_t type):
        type(type),
        code(0),
        data1(NULL)
    {}

    event::event(uint32_t type, uint32_t code, void* data1):
        type(type),
        code(code),
        data1(data1)
    {}

    event::event(const event& e):
        type(e.type),
        code(e.code),
        data1(e.data1)
    {}

}
