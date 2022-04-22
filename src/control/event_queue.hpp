#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>

#include "control/event.hpp"

namespace Control {

    //TODO this should be a ringbuffer, primary goal is reducing wait times for anyone pushing events into it as far as possible
    // make sure to move pushed and popped elements, make this a proper producer-consumer semaphore

    struct event_queue {
        std::mutex m;
        std::deque<event> q;
        std::condition_variable cv;
        void push(event e);
        event pop(uint32_t timeout_ms = 0); // wait until timeout or event to pop available, non blocking if 0, returns NULL event if none available
        event peek();
        uint32_t size();
        void clear();
    };

}
