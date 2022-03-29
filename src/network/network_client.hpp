#pragma once

#include <cstdint>
#include <thread>

#include "SDL_net.h"

#include "control/event_queue.hpp"
#include "control/timeout_crash.hpp"
#include "meta_gui/meta_gui.hpp"

namespace Network {

    class NetworkClient {
        private:
            Control::TimeoutCrash* tc; // we don't own this
            Control::TimeoutCrash::timeout_info tc_info;

            uint32_t log_id;

            std::thread send_runner;
            std::thread recv_runner;

            char* server_address;
            uint16_t server_port;
            IPaddress server_ip;
            TCPsocket socket = NULL;
            SDLNet_SocketSet socketset = NULL;

            uint32_t client_id = 0;

        public:
            Control::event_queue send_queue;
            Control::event_queue* recv_queue;

            NetworkClient(Control::TimeoutCrash* use_tc);
            ~NetworkClient();

            bool open(const char* host_address, uint16_t host_port);
            void close();

            void send_loop();
            void recv_loop();
    };

}
