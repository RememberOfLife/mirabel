#pragma once

#include <cstdint>
#include <thread>

#include "SDL_net.h"

#include "control/event_queue.hpp"

namespace Network {

    class NetworkClient {
        private:
            std::thread send_runner;
            std::thread recv_runner;

            IPaddress server_ip;
            TCPsocket socket = NULL;
            SDLNet_SocketSet socketset = NULL;

            uint32_t client_id = 0;

        public:
            Control::event_queue send_queue;
            Control::event_queue* recv_queue;

            NetworkClient();
            ~NetworkClient();

            bool open(const char* host_address, uint16_t host_port);
            void close();

            void send_loop();
            void recv_loop();
    };

}
