#include <cstdint>
#include <cstdio>

#include <SDL2/SDL.h>
#include "SDL_net.h"

#include "network/network_server.hpp"
#include "state_control/event.hpp"

#include "state_control/server.hpp"

namespace StateControl {

    //TODO all networking code here will eventually move to the networking adapter

    static uint32_t connection_count = 0;
    static uint32_t active_connection_id = 0;

    static uint16_t server_port = 61801;
    static IPaddress server_ip;
    static TCPsocket serve_sock = NULL;
    static SDLNet_SocketSet socketset = NULL;

    struct connection {
        TCPsocket sock;
        IPaddress peer;
    };
    static connection the_conn = {NULL};

    Server::Server()
    {
        //TODO no setup and cleanup for sdl+net if running offline
        // setup SDL
        if ( SDL_Init(0) < 0 ) {
            fprintf(stderr, "[FATAL] sdl init error: %s\n", SDL_GetError());
            exit(1);
        }
        // setup SDL_net
        if ( SDLNet_Init() < 0 ) {
            SDL_Quit();
            fprintf(stderr, "[FATAL] sdl_net init error: %s\n", SDLNet_GetError());
            exit(1);
        }
        
        Network::NetworkServer* net_server = new Network::NetworkServer();
        if (!net_server->open(NULL, 61801)) {
            fprintf(stderr, "[FATAL] networkserver failed to open\n");
            exit(1);
        }
        t_network = net_server;
        net_server->recv_queue = &inbox;
        inbox.push(event(EVENT_TYPE_NETWORK_ADAPTER_LOAD));

        printf("[INFO] networkserver constructed\n");
    }

    Server::~Server()
    {
        printf("[INFO] server shutting down\n");
        t_network->close();
        delete t_network;
        SDLNet_Quit();
        SDL_Quit();
    }

    void Server::loop()
    {
        bool quit = false;
        while (!quit) {
            event e = inbox.pop(UINT32_MAX);
            switch (e.type) {
                case EVENT_TYPE_NULL: {
                    printf("[WARN] received impossible null event\n");
                } break;
                case EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                //TODO heartbeat
                case EVENT_TYPE_NETWORK_ADAPTER_LOAD: {
                    if (t_network != NULL) {
                        network_send_queue = &(t_network->send_queue);
                        printf("[INFO] networkserver adapter loaded\n");
                    }
                } break;
                default: {
                    printf("[WARN] received unexpeted event, type: %d\n", e.type);
                } break;
            }
        }
        printf("[INFO] server exiting main loop\n");
    }

}
