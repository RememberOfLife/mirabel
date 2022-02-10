#pragma once

#include <deque>
#include <mutex>

#include "state_control/event.hpp"

namespace StateControl {

    //TODO this should be a ringbuffer, primary goal is reducing wait times for anyone pushing events into it as far as possible

    struct event_queue {
        std::mutex m;
        std::deque<event> q;
        void push(event e);
        event pop();
        event peek();
        uint32_t size();
        void clear();
    };

}
