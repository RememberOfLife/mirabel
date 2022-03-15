#pragma once

#include <thread>

#include "control/event_queue.hpp"

namespace Control {

    class TimeoutCrashThread {

        //TODO use a condition variable, inbox checking can be moved to just the timeout_ms interval, exiting becomes actually responsive

        public:

            int initial_delay = 3000;
            int timeout_ms = 1000;

            std::thread runner;
            event_queue inbox;

            TimeoutCrashThread();
            ~TimeoutCrashThread();
            void loop();

            void start();
            void join();

    };

}
