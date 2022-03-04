#pragma once

#include <thread>

#include "state_control/event_queue.hpp"

namespace StateControl {

    class TimeoutCrashThread {

        public:

            int initial_delay = 3000;
            int timeout_ms = 1000;

            std::thread running_thread;
            event_queue inbox;

            TimeoutCrashThread();
            ~TimeoutCrashThread();
            void loop();

            void start();
            void join();

    };

}
