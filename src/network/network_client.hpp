#pragma once

#include <cstdint>
#include <thread>

#include "SDL_net.h"
#include <openssl/ssl.h>

#include "control/event_queue.hpp"
#include "control/timeout_crash.hpp"
#include "meta_gui/meta_gui.hpp"
#include "network/protocol.hpp"
#include "network/util.hpp"

namespace Network {

    class NetworkClient {
        private:
            Control::TimeoutCrash* tc; // we don't own this
            Control::TimeoutCrash::timeout_info tc_info;

            uint32_t log_id;

            //TODO figure out how the network client pushes out state updates for the connection metagui window, just use the recv queue?
            
            std::thread send_runner;
            std::thread recv_runner;

            SSL_CTX* ssl_ctx;

            char* server_address;
            uint16_t server_port;
            SDLNet_SocketSet socketset = NULL;
            connection conn; // client id starts out as 0 before reassignment

        public:
            Control::event_queue send_queue;
            Control::event_queue* recv_queue;

            //TODO atomic ping and heartbeat times (hb might go into event)

            NetworkClient(Control::TimeoutCrash* use_tc);
            ~NetworkClient();

            bool open(const char* host_address, uint16_t host_port);
            void close();

            void send_loop();
            void recv_loop();
    };

}
