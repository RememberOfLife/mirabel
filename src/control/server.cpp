#include <cstdint>
#include <cstdio>
#include <cstring>

#include <SDL2/SDL.h>
#include "SDL_net.h"
#include "rosalia/rand.h"
#include "rosalia/semver.h"
#include "surena/game.h"

#include "mirabel/event.h"
#include "control/auth_manager.hpp"
#include "control/lobby_manager.hpp"
#include "control/plugins.hpp"
#include "control/user_manager.hpp"
#include "network/network_server.hpp"

#include "control/server.hpp"

namespace Control {

    const semver server_version = semver{0, 2, 1};

    Server::Server():
        plugin_mgr(true, false)
    {
        event_queue_create(&inbox);

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
        event_any es;
        event_create_type(&es, EVENT_TYPE_NETWORK_ADAPTER_LOAD);
        event_queue_push(&inbox, &es);

        printf("[INFO] networkserver constructed\n");

        // detect plugins in plugin_mgr and load all
        plugin_mgr.detect_plugins();
        for (int i = 0; i < plugin_mgr.plugins.size(); i++) {
            plugin_mgr.load_plugin(i);
        }

        lobby_mgr.plugin_mgr = &plugin_mgr;
        lobby_mgr.send_queue = network_send_queue;
        auth_mgr.send_queue = network_send_queue;
    }

    Server::~Server()
    {
        tc_info.pre_quit(2000);

        printf("[INFO] server shutting down\n");
        if (t_network) {
            t_network->close();
            delete t_network;
        }
        SDLNet_Quit();
        SDL_Quit();

        t_tc.unregister_timeout_item(tc_info.id);
        event_any es;
        event_create_type(&es, EVENT_TYPE_EXIT);
        event_queue_push(&t_tc.inbox, &es);
        t_tc.join();

        event_queue_destroy(&inbox);
    }

    void Server::loop()
    {
        event_any e;
        bool quit = false;
        while (!quit) {
            event_queue_pop(&inbox, &e, UINT32_MAX);
            switch (e.base.type) {
                case EVENT_TYPE_NULL: {
                    printf("[WARN] received impossible null event\n");
                } break;
                case EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                case EVENT_TYPE_LOG: {
                    printf("%s", e.log.str);
                } break;
                case EVENT_TYPE_HEARTBEAT: {
                    tc_info.send_heartbeat();
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED: {
                    //TODO usermgr
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED: {
                    //TODO usermgr
                } break;
                case EVENT_TYPE_GAME_LOAD:
                case EVENT_TYPE_GAME_UNLOAD:
                case EVENT_TYPE_GAME_STATE:
                case EVENT_TYPE_GAME_MOVE:
                case EVENT_TYPE_GAME_SYNC:
                case EVENT_TYPE_LOBBY_CREATE:
                case EVENT_TYPE_LOBBY_DESTROY:
                case EVENT_TYPE_LOBBY_JOIN:
                case EVENT_TYPE_LOBBY_LEAVE:
                case EVENT_TYPE_LOBBY_INFO:
                case EVENT_TYPE_LOBBY_CHAT_MSG:
                case EVENT_TYPE_LOBBY_CHAT_DEL: {
                    lobby_mgr.HandleEvent(e);
                } break;
                case EVENT_TYPE_USER_AUTHINFO: {
                    // client wants to have the authinfo, serve it
                    event_any es;
                    event_create_auth(&es, EVENT_TYPE_USER_AUTHINFO, e.base.client_id, true, NULL, NULL);
                    event_queue_push(network_send_queue, &es);
                    //TODO it should be *possible* for the server to respond to a authinfo event with a login confirmation
                } break;
                case EVENT_TYPE_USER_AUTHN: {
                    event_any es;
                    // client wants to auth with given credentials, send back authn or authfail
                    //TODO save guest names and check for dupes
                    if (!e.auth.is_guest) {
                        event_create_auth_fail(&es, e.base.client_id, "user logins not accepted");
                        event_queue_push(network_send_queue, &es);
                        break;
                    }
                    if (e.auth.username == NULL) {
                        event_create_auth_fail(&es, e.base.client_id, "name NULL");
                        event_queue_push(network_send_queue, &es);
                        break;
                    }
                    // validate that username uses only allowed characters
                    for (int i = 0; i < strlen(e.auth.username); i++) {
                        if (!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", e.auth.username[i])) {
                            event_create_auth_fail(&es, e.base.client_id, "name contains illegal characters");
                            event_queue_push(network_send_queue, &es);
                            break;
                        }
                    }
                    if (strlen(e.auth.username) > 0 && strlen(e.auth.username) < 3) {
                        event_create_auth_fail(&es, e.base.client_id, "name < 3 characters");
                        event_queue_push(network_send_queue, &es);
                        break;
                    }
                    if (strlen(e.auth.username) == 0) {
                        free(e.auth.username);
                        static uint32_t seed = 123;
                        fast_prng rng;
                        fprng_srand(&rng, seed++);
                        const int assigned_length = 5;
                        const int guestname_length = 6 + assigned_length;
                        e.auth.username = (char*)malloc(guestname_length);
                        char* str_p = e.auth.username;
                        str_p += sprintf(str_p, "Guest");
                        for (int i = 0; i < assigned_length; i++) {
                            str_p += sprintf(str_p, "%d", fprng_rand(&rng) % 10);
                        }
                    }
                    event_create_auth(&es, EVENT_TYPE_USER_AUTHN, e.base.client_id, true, e.auth.username, NULL);
                    event_queue_push(network_send_queue, &es);
                } break;
                case EVENT_TYPE_USER_AUTHFAIL: {
                    // client wants to logout but keep the connection, we tell them we logged them out
                    event_any es;
                    event_create_auth_fail(&es, e.base.client_id, NULL);
                    event_queue_push(network_send_queue, &es);
                } break;
                case EVENT_TYPE_NETWORK_ADAPTER_LOAD: {
                    if (t_network != NULL) {
                        network_send_queue = &(t_network->send_queue);
                        printf("[INFO] networkserver adapter loaded\n");
                        lobby_mgr.send_queue = network_send_queue; //TODO is this refresh actually required?
                        auth_mgr.send_queue = network_send_queue;
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
            event_destroy(&e);
        }
        printf("[INFO] server exiting main loop\n");
    }

} // namespace Control
