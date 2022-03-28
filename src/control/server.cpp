#include <cstdint>
#include <cstdio>

#include <SDL2/SDL.h>
#include "SDL_net.h"
#include "surena/game.hpp"

#include "control/event.hpp"
#include "network/network_server.hpp"

#include "control/server.hpp"

namespace Control {

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
        // start watchdog so it can oversee explicit construction
        t_tc.start();
        tc_info = t_tc.register_timeout_item(&inbox, "guithread", 3000, 1000);

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
        t_tc.unregister_timeout_item(tc_info.id);

        printf("[INFO] server shutting down\n");
        t_network->close();
        delete t_network;
        SDLNet_Quit();
        SDL_Quit();

        //TODO maybe this should properly oversee destruction aswell
        t_tc.inbox.push(EVENT_TYPE_EXIT);
        t_tc.join();
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
                case EVENT_TYPE_HEARTBEAT: {
                    tc_info.send_heartbeat();
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED: {
                    // we only have one lobby for now
                    if (lobby) {
                        lobby->AddUser(e.client_id);
                    }
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED: {
                    // we only have one lobby for now
                    if (lobby) {
                        lobby->RemoveUser(e.client_id);
                    }
                } break;
                case EVENT_TYPE_GAME_LOAD:
                case EVENT_TYPE_GAME_UNLOAD:
                case EVENT_TYPE_GAME_IMPORT_STATE:
                case EVENT_TYPE_GAME_MOVE:
                case EVENT_TYPE_LOBBY_CHAT_MSG:
                case EVENT_TYPE_LOBBY_CHAT_DEL: {
                    // we only have one lobby for now
                    if (lobby) {
                        lobby->HandleEvent(e);
                    }
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_LOAD: {
                    if (t_network != NULL) {
                        network_send_queue = &(t_network->send_queue);
                        printf("[INFO] networkserver adapter loaded\n");
                        //TODO creating the lobby here is very ugly
                        lobby = new Lobby(network_send_queue, 2);
                        printf("[INFO] lobby created\n");
                    }
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED: {
                    // network adapter died or closed
                    // crash fatal for now
                    printf("[FATAL] networkserver died\n");
                    exit(1);
                } break;
                default: {
                    printf("[WARN] received unexpeted event, type: %d\n", e.type);
                } break;
            }
        }
        printf("[INFO] server exiting main loop\n");
    }

}
