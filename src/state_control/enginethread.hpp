#pragma once

#include <thread>

#include "surena_game.hpp"

#include "state_control/event_queue.hpp"

namespace StateControl {

    class EngineThread {

        public:

            std::thread running_thread;
            event_queue inbox;

            surena::PerfectInformationGame* game;

            EngineThread();
            ~EngineThread();
            void loop();

            void start();
            void join();

    };

}
