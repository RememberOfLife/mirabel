#pragma once

#include <cstdint>
#include <thread>

#include "SDL_net.h"

#include "state_control/event_queue.hpp"

namespace Network {

    class NetworkServer {
        private:
            std::thread server_runner;
            //TODO these may also be multiple runners, split among shards of the connected clients
            std::thread send_runner;
            std::thread recv_runner;

            // server socket
            IPaddress server_ip;
            TCPsocket server_socket = NULL;
            SDLNet_SocketSet server_socketset = NULL;
            // connected client sockets
            uint32_t client_connection_bucket_size = 2; // must be <= UINT32_MAX-2
            struct client_connection {
                TCPsocket socket;
                IPaddress peer;
                uint32_t client_id;
            };
            client_connection* client_connections = NULL;
            SDLNet_SocketSet client_socketset = NULL;

            //TODO adding a newly connected client to the client socketset is probably not threadsafe with the recv_runner that is waiting for the set
            // most other components of the network server are likely not threadsafe either

            //TODO test the proper self exit of this in the server, -> deconstruction and cleanup

            //TODO struct for connected client
            //TODO doubly linked list for client activity

        public:
            StateControl::event_queue send_queue;
            StateControl::event_queue* recv_queue;

            NetworkServer();
            ~NetworkServer();

            bool open(const char* host_address, uint16_t host_port);
            void close();

            void server_loop();
            void send_loop();
            void recv_loop();
    };

}
