#pragma once

#include <cstdint>
#include <thread>

#include "SDL_net.h"
#include <openssl/ssl.h>

#include "mirabel/event_queue.h"
#include "network/util.hpp"

namespace Network {

    class NetworkServer {
      private:

        //TODO use timeoutcrash

        std::thread server_runner;
        //TODO these may also be multiple runners, split among shards of the connected clients
        std::thread send_runner;
        std::thread recv_runner;

        SSL_CTX* ssl_ctx;

        // server socket
        IPaddress server_ip;
        TCPsocket server_socket = NULL;
        SDLNet_SocketSet server_socketset = NULL;
        // connected client sockets
        uint32_t client_connection_bucket_size = 256; // must be <= UINT32_MAX-2 //TODO set higher for proper use
        connection* client_connections = NULL;
        SDLNet_SocketSet client_socketset = NULL;

        //TODO adding a newly connected client to the client socketset is probably not threadsafe with the recv_runner that is waiting for the set
        // most other components of the network server (and maybe client) are likely not threadsafe either

        //TODO test the proper self exit of this in the server, -> deconstruction and cleanup

        //TODO doubly linked list for client activity + client counter per bucket

      public:

        event_queue send_queue;
        event_queue* recv_queue;

        NetworkServer();
        ~NetworkServer();

        bool open(const char* host_address, uint16_t host_port);
        void close();

        void server_loop();
        void send_loop();
        void recv_loop();
    };

} // namespace Network
