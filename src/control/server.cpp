#include <cstdint>
#include <cstdio>
#include <cstring>

#include <SDL2/SDL.h>
#include "SDL_net.h"
#include "surena/util/fast_prng.hpp"
#include "surena/game.h"

#include "control/event.h"
#include "network/network_server.hpp"

#include "control/server.hpp"

namespace Control {

    Server::Server()
    {
        f_event_queue_create(&inbox);

        // start watchdog so it can oversee explicit construction
        t_tc.start();
        tc_info = t_tc.register_timeout_item(&inbox, "guithread", 3000, 1000);

        //TODO no setup and cleanup for sdl+net if running offline
        // setup SDL
        if (SDL_Init(0) < 0) {
            fprintf(stderr, "[FATAL] sdl init error: %s\n", SDL_GetError());
            exit(1);
        }
        // setup SDL_net
        if (SDLNet_Init() < 0) {
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
        f_event_any es;
        f_event_create_type(&es, EVENT_TYPE_NETWORK_ADAPTER_LOAD);
        f_event_queue_push(&inbox, &es);

        printf("[INFO] networkserver constructed\n");
    }

    Server::~Server()
    {
        tc_info.pre_quit(2000);

        printf("[INFO] server shutting down\n");
        delete lobby;
        if (t_network) {
            t_network->close();
            delete t_network;
        }
        SDLNet_Quit();
        SDL_Quit();

        t_tc.unregister_timeout_item(tc_info.id);
        f_event_any es;
        f_event_create_type(&es, EVENT_TYPE_EXIT);
        f_event_queue_push(&t_tc.inbox, &es);
        t_tc.join();

        f_event_queue_destroy(&inbox);
    }

    void Server::loop()
    {
        f_event_any e;
        bool quit = false;
        while (!quit) {
            f_event_queue_pop(&inbox, &e, UINT32_MAX);
            switch (e.base.type) {
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
                        lobby->AddUser(e.base.client_id);
                    }
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED: {
                    // we only have one lobby for now
                    if (lobby) {
                        lobby->RemoveUser(e.base.client_id);
                    }
                } break;
                case EVENT_TYPE_GAME_LOAD:
                case EVENT_TYPE_GAME_UNLOAD:
                case EVENT_TYPE_GAME_STATE:
                case EVENT_TYPE_GAME_MOVE:
                case EVENT_TYPE_LOBBY_CHAT_MSG:
                case EVENT_TYPE_LOBBY_CHAT_DEL: {
                    // we only have one lobby for now
                    if (lobby) {
                        lobby->HandleEvent(e);
                    }
                } break;
                case EVENT_TYPE_USER_AUTHINFO: {
                    // client wants to have the authinfo, serve it
                    f_event_any es;
                    f_event_create_auth(&es, EVENT_TYPE_USER_AUTHINFO, e.base.client_id, true, NULL, NULL);
                    f_event_queue_push(network_send_queue, &es);
                } break;
                case EVENT_TYPE_USER_AUTHN: {
                    auto ce = event_cast<f_event_auth>(e);
                    f_event_any es;
                    // client wants to auth with given credentials, send back authn or authfail
                    //TODO save guest names and check for dupes
                    if (!ce.is_guest) {
                        f_event_create_auth_fail(&es, e.base.client_id, "user logins not accepted");
                        f_event_queue_push(network_send_queue, &es);
                        break;
                    }
                    if (ce.username == NULL) {
                        f_event_create_auth_fail(&es, e.base.client_id, "name NULL");
                        f_event_queue_push(network_send_queue, &es);
                        break;
                    }
                    // validate that username uses only allowed characters
                    for (int i = 0; i < strlen(ce.username); i++) {
                        if (!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", ce.username[i])) {
                            f_event_create_auth_fail(&es, e.base.client_id, "name contains illegal characters");
                            f_event_queue_push(network_send_queue, &es);
                            break;
                        }
                    }
                    if (strlen(ce.username) > 0 && strlen(ce.username) < 3) {
                        f_event_create_auth_fail(&es, e.base.client_id, "name < 3 characters");
                        f_event_queue_push(network_send_queue, &es);
                        break;
                    }
                    if (strlen(ce.username) == 0) {
                        free(ce.username);
                        static fast_prng rng(123);
                        const int assigned_length = 5;
                        const int guestname_length = 6+assigned_length;
                        ce.username = (char*)malloc(guestname_length);
                        char* str_p = ce.username;
                        str_p += sprintf(str_p, "Guest");
                        for (int i = 0; i < assigned_length; i++) {
                            str_p += sprintf(str_p, "%d", rng.rand()%10);
                        }
                    }
                    f_event_create_auth(&es, EVENT_TYPE_USER_AUTHN, e.base.client_id, true, ce.username, NULL);
                    f_event_queue_push(network_send_queue, &es);
                } break;
                case EVENT_TYPE_USER_AUTHFAIL: {
                    // client wants to logout but keep the connection, we tell them we logged them out
                    f_event_any es;
                    f_event_create_auth_fail(&es, e.base.client_id, NULL);
                    f_event_queue_push(network_send_queue, &es);
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
                    printf("[WARN] received unexpected event, type: %d\n", e.base.type);
                } break;
            }
            f_event_destroy(&e);
        }
        printf("[INFO] server exiting main loop\n");
    }

}
