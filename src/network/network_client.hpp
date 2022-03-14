#pragma once

#include <thread>

#include "SDL_net.h"

#include "state_control/event_queue.hpp"

namespace Network {

    class NetworkClient {
        private:
            std::thread send_runner;
            std::thread recv_runner;

            IPaddress server_ip;
            TCPsocket socket = NULL;
            SDLNet_SocketSet socketset = NULL;

        public:
            StateControl::event_queue send_queue;
            StateControl::event_queue* recv_queue;

            NetworkClient();
            ~NetworkClient();

            bool open(const char* host_address);
            void close();

            void send_loop();
            void recv_loop();
    };

}
