#include <cstdint>
#include <cstdlib>
#include <thread>

#include "SDL_net.h"

#include "meta_gui/meta_gui.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"

#include "network/network_client.hpp"

namespace Network {

    NetworkClient::NetworkClient()
    {
        socketset = SDLNet_AllocSocketSet(1);
        if (socketset == NULL) {
            MetaGui::log("#E networkclient: failed to allocate socketset\n");
        }
    }

    NetworkClient::~NetworkClient()
    {
        SDLNet_FreeSocketSet(socketset);
    }

    bool NetworkClient::open(const char* host_address, uint16_t host_port)
    {
        if (socketset == NULL) {
            return false;
        }
        if (SDLNet_ResolveHost(&server_ip, host_address, host_port)) {
            MetaGui::log("#W networkclient: could not resolve host address\n");
            return false;
        }
        socket = SDLNet_TCP_Open(&server_ip);
        if (socket == NULL) {
            MetaGui::log("#W networkclient: socket failed to open\n");
            return false;
        }
        SDLNet_TCP_AddSocket(socketset, socket); // cant fail, we only have one socket for our size 1 set
        send_runner = std::thread(&NetworkClient::send_loop, this); // socket open, start send_runner
        recv_runner = std::thread(&NetworkClient::recv_loop, this); // socket open, start recv_runner
        return true;
    }

    void NetworkClient::close()
    {
        send_queue.push(Control::event(Control::EVENT_TYPE_EXIT)); // stop send_runner
        // stop recv_runner
        SDLNet_TCP_DelSocket(socketset, socket);
        SDLNet_TCP_Close(socket);
        socket = NULL;
        // everything closed, join dead runners
        send_runner.join();
        recv_runner.join();
    }

    void NetworkClient::send_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer = (uint8_t*)malloc(buffer_size); // recycled buffer for outgoing data, if something requires more, alloc it specially
        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer);
        
        // wait until event available
        bool quit = false;
        while (!quit && socket != NULL) {
            Control::event e = send_queue.pop(UINT32_MAX);
            switch (e.type) {
                case Control::EVENT_TYPE_NULL: {
                    MetaGui::log("#W networkclient: received impossible null event\n");
                } break;
                case Control::EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                //TODO heartbeat
                default: {
                    //TODO universal event->packet encoding
                    MetaGui::logf("#I networkclient: send event, type: %d\n", e.type);
                    *db_event_type = e.type;
                    *(db_event_type+1) = client_id;
                    int send_len = sizeof(Control::event);
                    int sent_len = SDLNet_TCP_Send(socket, data_buffer, send_len);
                    if (sent_len != send_len) {
                        MetaGui::log("#W networkclient: packet sending failed\n");
                    }
                } break;
            }
        }

        free(data_buffer);
    }

    void NetworkClient::recv_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer_base = (uint8_t*)malloc(buffer_size); // recycled buffer for incoming data

        while (socket != NULL) {
            int ready = SDLNet_CheckSockets(socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            if (ready == -1) {
                break;
            }
            if (SDLNet_SocketReady(socket)) {
                int recv_len = SDLNet_TCP_Recv(socket, data_buffer_base, buffer_size);
                if (recv_len <= 0) {
                    // connection closed
                    SDLNet_TCP_DelSocket(socketset, socket);
                    SDLNet_TCP_Close(socket);
                    socket = NULL;
                    MetaGui::log("#W networkclient: connection closed unexpectedly\n");
                    break;
                } else {
                    // one call to recv may receive MULTIPLE packets at once, process them all
                    uint8_t* data_buffer = data_buffer_base;
                    while (true) {
                        if (recv_len < sizeof(Control::event)) {
                            MetaGui::logf("#W networkclient: discarding %d unusable bytes of received data\n", recv_len);
                            break;
                        }
                        // at least one packet here, process it from data_buffer
                        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer);
                        // switch on type
                        switch (*db_event_type) {
                            case Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET: {
                                client_id = *(db_event_type+1);
                                MetaGui::logf("#I networkclient: assigned client id %d\n", client_id);
                            } break;
                            default: {
                                //TODO universal packet->event decoding, then place it in the recv_queue
                                MetaGui::logf("#I networkclient: received event, type: %d\n", *db_event_type);
                            } break;
                        }
                        data_buffer += sizeof(Control::event);
                        recv_len -= sizeof(Control::event);
                        if (recv_len == 0) {
                            break;
                        }
                    }
                }
            }
        }

        free(data_buffer_base);
        // if the connection is closed, enqueue a connection closed event to ensure the gui resets its connection
        recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSE));
    }
    
}
