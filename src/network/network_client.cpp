#include "SDL_net.h"

#include "meta_gui/meta_gui.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"

#include "network/network_client.hpp"

namespace Network {

    NetworkClient::NetworkClient()
    {
        socketset = SDLNet_AllocSocketSet(1);
    }

    NetworkClient::~NetworkClient()
    {
        SDLNet_FreeSocketSet(socketset);
    }

    bool NetworkClient::open(const char* host_address)
    {
        SDLNet_ResolveHost(&server_ip, host_address, 61801);
        socket = SDLNet_TCP_Open(&server_ip);
        if (socket != NULL) {
            SDLNet_TCP_AddSocket(socketset, socket);
            send_runner = std::thread(&NetworkClient::send_loop, this); // socket open, start send_runner
            recv_runner = std::thread(&NetworkClient::recv_loop, this); // socket open, start recv_runner
        } else {
            MetaGui::log("#W networkclient: socket failed to open\n");
            return false;
        }
        return true;
    }

    void NetworkClient::close()
    {
        send_queue.push(StateControl::event(StateControl::EVENT_TYPE_EXIT)); // stop send_runner
        // stop recv_runner
        SDLNet_TCP_Close(socket);
        SDLNet_TCP_DelSocket(socketset, socket);
        socket = NULL;
        send_runner.join(); // socket closed, join dead send_runner
        recv_runner.join(); // socket closed, join dead recv_runner
    }

    void NetworkClient::send_loop()
    {
        // wait until event available
        bool quit = false;
        while (!quit) {
            StateControl::event e = send_queue.pop(UINT32_MAX);
            switch (e.type) {
                case StateControl::EVENT_TYPE_NULL: {
                    MetaGui::log("#W networkclient: received impossible null event\n");
                } break;
                case StateControl::EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                //TODO heartbeat
                default: {
                    MetaGui::logf("#I networkclient: send event, type: %d\n", e.type);
                } break;
            }
        }
    }

    void NetworkClient::recv_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer = (uint8_t*)malloc(buffer_size); // recylced buffer for incoming data
        while (socket != NULL) {
            int ready = SDLNet_CheckSockets(socketset, 10); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            if (ready == -1) {
                break;
            }
            if (SDLNet_SocketReady(socket)) {
                int recv_len = SDLNet_TCP_Recv(socket, data_buffer, buffer_size);
                if (recv_len <= 0) {
                    // connection closed
                    SDLNet_TCP_DelSocket(socketset, socket);
                    SDLNet_TCP_Close(socket);
                    socket = NULL;
                    break;
                } else {
                    // data received, switch on it

                }
            }
        }
        free(data_buffer);
        // if the connection is closed, enqueue a connection closed event to ensure the gui resets its connection
        recv_queue->push(StateControl::event(StateControl::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSE));
    }
    
}
