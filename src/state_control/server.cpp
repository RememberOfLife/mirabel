#include <cstdint>

#include "SDL2/SDL.h"
#include "SDL_net.h"

#include "state_control/server.hpp"

namespace StateControl {

    //TODO all networking code here will eventually move to the networking adapter

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
        // setup SDL
        if ( SDL_Init(0) < 0 ) {
            fprintf(stderr, "[FATAL] sdl init error: %s\n", SDL_GetError());
            exit(-1);
        }
        // setup SDL_net
        if ( SDLNet_Init() < 0 ) {
            SDL_Quit();
            fprintf(stderr, "[FATAL] sdl_net init error: %s\n", SDLNet_GetError());
            exit(-1);
        }
        socketset = SDLNet_AllocSocketSet(2);
        if (socketset == NULL) {
            fprintf(stderr, "[FATAL] could not create socket set: %s\n", SDLNet_GetError());
            exit(-1);
        }
        if (SDLNet_ResolveHost(&server_ip, NULL, server_port) != 0) {
            fprintf(stderr, "[FATAL] could not resolve server address\n");
        }
        // server_ip.host = INADDR_LOOPBACK;
        // server_ip.port = server_port;
        uint8_t address_dec[4];
        for (int i = 0; i < 4; i++) {
            address_dec[i] = (server_ip.host>>(8*(3-i)))&0xFF;
        }
        printf("starting server on ip: %d.%d.%d.%d:%d\n", address_dec[0], address_dec[1], address_dec[2], address_dec[3], server_ip.port);
        serve_sock = SDLNet_TCP_Open(&server_ip);
        if (serve_sock == NULL) {
            fprintf(stderr, "[FATAL] could not create server socket: %s\n", SDLNet_GetError());
            exit(-1);
        }
        SDLNet_TCP_AddSocket(socketset, serve_sock);

        printf("init finished, listening..\n");
    }

    Server::~Server()
    {
        if (serve_sock != NULL) {
            SDLNet_TCP_Close(serve_sock);
            serve_sock = NULL;
        }
        if (socketset != NULL) {
            SDLNet_FreeSocketSet(socketset);
            socketset = NULL;
        }
        SDLNet_Quit();
        SDL_Quit();
    }

    void Server::loop()
    {
        bool quit = false;
        while (!quit) {
            // wait until something happens on the sockets
            SDLNet_CheckSockets(socketset, UINT32_MAX);

            // handle new connection
            if (SDLNet_SocketReady(serve_sock)) {
                TCPsocket incoming_sock;
                incoming_sock = SDLNet_TCP_Accept(serve_sock);
                if (incoming_sock != NULL) {
                    uint8_t data = BP_OK;
                    if (the_conn.sock != NULL) {
                        // already have a conn, refuse new connection
                        data = BP_NOK;
                        SDLNet_TCP_Send(incoming_sock, &data, 1);
                        SDLNet_TCP_Close(incoming_sock);
                        printf("refused new connection\n");
                    } else {
                        // accept incoming connection and place it in the connection slot
                        SDLNet_TCP_Send(incoming_sock, &data, 1);
                        the_conn.sock = incoming_sock;
                        the_conn.peer = *SDLNet_TCP_GetPeerAddress(incoming_sock);
                        SDLNet_TCP_AddSocket(socketset, the_conn.sock);
                        printf("accepted new connection\n");
                    }
                }
            }

            // check for anything new on client
            if (SDLNet_SocketReady(the_conn.sock)) {
                uint8_t in_data[512]; // data buffer for incoming data
                uint8_t out_data[512];
                if ( SDLNet_TCP_Recv(the_conn.sock, in_data, 512) <= 0 ) {
                    // connection closed
                    printf("connection closed\n");
                    SDLNet_TCP_DelSocket(socketset, the_conn.sock);
                    SDLNet_TCP_Close(the_conn.sock);
                    the_conn.sock = NULL;
                } else {
                    switch (in_data[0]) {
                        case BP_PING: {
                            printf("received ping, sending pong\n");
                            out_data[0] = BP_PONG;
                            SDLNet_TCP_Send(the_conn.sock, out_data, 1);
                        } break;
                        case BP_TEXT: {
                            in_data[511] = '\0'; // force a nullbyte at the end of the message
                            printf("received text: %s\n", in_data+1);
                            out_data[0] = BP_OK;
                            SDLNet_TCP_Send(the_conn.sock, out_data, 1);
                        } break;
                        default: {
                            printf("received unexpected packet type\n");
                        } break;
                    }
                }
            }
        }
    }

}
