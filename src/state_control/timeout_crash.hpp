#pragma once

#include <thread>

#include "state_control/event_queue.hpp"

namespace StateControl {

    class TimeoutCrashThread {

        public:

            uint64_t initial_sleep = 3000;
            uint64_t timeout_ms = 3000;

            std::thread running_thread;
            event_queue inbox;

            TimeoutCrashThread();
            ~TimeoutCrashThread();
            void loop();

            void start();
            void join();

    };

}
