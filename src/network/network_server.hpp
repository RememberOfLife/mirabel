#pragma once

#include <thread>

#include "state_control/event_queue.hpp"

namespace Network {

    class NetworkServer {
        private:
            StateControl::event_queue send_queue;
            StateControl::event_queue* recv_queue;

            std::thread runner;

            //TODO the actual network socket

        public:
            NetworkServer();
            ~NetworkServer();

            void loop();
            void start();
            void join();
    };

}
