#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "SDL_net.h"

#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "meta_gui/meta_gui.hpp"

#include "network/network_client.hpp"

namespace Network {

    NetworkClient::NetworkClient(Control::TimeoutCrash* use_tc)
    {
        log_id = MetaGui::log_register("NetworkClient");
        if (use_tc) {
            tc = use_tc;
        }
        socketset = SDLNet_AllocSocketSet(1);
        if (socketset == NULL) {
            MetaGui::log(log_id, "#E failed to allocate socketset\n");
        }
    }

    NetworkClient::~NetworkClient()
    {
        free(server_address);
        SDLNet_FreeSocketSet(socketset);
        MetaGui::log_unregister(log_id);
    }

    bool NetworkClient::open(const char* host_address, uint16_t host_port)
    {
        if (socketset == NULL) {
            return false;
        }
        server_address = (char*)malloc(strlen(host_address)+1);
        strcpy(server_address, host_address);
        server_port = host_port;
        send_runner = std::thread(&NetworkClient::send_loop, this); // socket open, start send_runner
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
        send_runner.join(); // send_runner joins recv_runner for us
    }

    void NetworkClient::send_loop()
    {
        // open the socket
        if (SDLNet_ResolveHost(&server_ip, server_address, server_port)) {
            MetaGui::log(log_id, "#W could not resolve host address\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }
        socket = SDLNet_TCP_Open(&server_ip);
        if (socket == NULL) {
            MetaGui::log(log_id, "#W socket failed to open\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }
        SDLNet_TCP_AddSocket(socketset, socket); // cant fail, we only have one socket for our size 1 set

        if (tc) {
            tc_info = tc->register_timeout_item(&send_queue, "networkclient", 1000, 1000);
        }

        // everything fine, enter the actual send loop and start recv_runner
        recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED));
        recv_runner = std::thread(&NetworkClient::recv_loop, this); // socket open, start recv_runner

        uint32_t base_buffer_size = 1024;
        uint8_t* data_buffer_base = (uint8_t*)malloc(base_buffer_size); // recycled buffer for outgoing data

        // wait until event available
        bool quit = false;
        while (!quit && socket != NULL) {
            Control::event e = send_queue.pop(UINT32_MAX);
            switch (e.type) {
                case Control::EVENT_TYPE_NULL: {
                    MetaGui::log(log_id, "#W received impossible null event\n");
                } break;
                case Control::EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                case Control::EVENT_TYPE_HEARTBEAT: {
                    tc_info.send_heartbeat();
                } break;
                default: {
                    // universal event->packet encoding, for POD events
                    uint8_t* data_buffer = data_buffer_base;
                    e.client_id = client_id;
                    int send_len = sizeof(Control::event);
                    if (e.raw_data) {
                        if (e.raw_length > base_buffer_size-send_len) {
                            // if raw data is too big for our reusable buffer, malloc a fitting one
                            data_buffer = (uint8_t*)malloc(send_len+e.raw_length);
                        }
                        memcpy(data_buffer+send_len, e.raw_data, e.raw_length);
                        send_len += e.raw_length;
                    } else {
                        e.raw_length = 0; // not really necessary
                    }
                    free(e.raw_data);
                    e.raw_data = NULL; // security: don't expose internal pointer to raw data
                    memcpy(data_buffer, &e, sizeof(Control::event));
                    int sent_len = SDLNet_TCP_Send(socket, data_buffer, send_len);
                    if (sent_len != send_len) {
                        MetaGui::log(log_id, "#W packet sending failed\n");
                    }
                    if (data_buffer != data_buffer_base) {
                        free(data_buffer);
                    }
                    MetaGui::logf(log_id, "#I sent event, type %d, len %d\n", e.type, send_len);
                } break;
            }
        }

        if (tc) {
            tc_info.pre_quit(1000);
        }

        free(data_buffer_base);

        recv_runner.join(); // recv_runner might fail to join if it gets stuck

        if (tc) {
            tc->unregister_timeout_item(tc_info.id);
        }
    }

    void NetworkClient::recv_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer_base = (uint8_t*)malloc(buffer_size); // recycled buffer for incoming data

        while (socket != NULL) {
            int ready = SDLNet_CheckSockets(socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            //TODO if we have to go out of waiting anyway every once in a while, the maybe check a dedicated heartbeat inbox here too?
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
                    MetaGui::log(log_id, "#W connection closed unexpectedly\n");
                    break;
                } else {
                    // one call to recv may receive MULTIPLE events at once, process them all
                    uint8_t* data_buffer = data_buffer_base;
                    while (true) {
                        if (recv_len < sizeof(Control::event)) {
                            MetaGui::logf(log_id, "#W discarding %d unusable bytes of received data\n", recv_len);
                            break;
                        }
                        // universal packet->event decoding, then place it in the recv_queue
                        // at least one event here, process it from data_buffer
                        Control::event recv_event = Control::event();
                        memcpy(&recv_event, data_buffer, sizeof(Control::event));
                        // update size of remaining buffer
                        data_buffer += sizeof(Control::event);
                        recv_len -= sizeof(Control::event);
                        // if there is a raw data payload attached to the event, get that too
                        if (recv_event.raw_length > 0) {
                            recv_event.raw_data = malloc(recv_event.raw_length);
                            int raw_overhang = static_cast<int>(recv_event.raw_length) - recv_len; //HACK type horror here
                            if (raw_overhang <= 0) {
                                // whole raw data payload is captured in the remaining data buffer
                                // there might even be more packets behind that
                                memcpy(recv_event.raw_data, data_buffer, recv_event.raw_length);
                                data_buffer += recv_event.raw_length;
                                recv_len -= recv_event.raw_length;
                            } else {
                                // the current raw data segment is extending beyond the buffer we received
                                // we want to read ONLY those bytes of the raw data segment we're missing
                                // any events that may still be read after that will just fire SDLNet_CheckSockets 
                                // they will be processed in the next buffer charge
                                memcpy(recv_event.raw_data, data_buffer, recv_len); // memcpy all bytes left in the current data_buffer into raw data segment
                                // malloc space for the overhang data, and read exactly that
                                data_buffer = (uint8_t*)malloc(raw_overhang);
                                int overhang_recv_len = SDLNet_TCP_Recv(socket, data_buffer, raw_overhang);
                                if (overhang_recv_len != raw_overhang) {
                                    // did not receive all the missing bytes, might be disastrous for the event
                                    MetaGui::logf(log_id, "#E received only %d bytes of overhang, expected %d, event dropped\n", overhang_recv_len, raw_overhang);
                                    // drop event to null type, will be discarded later
                                    recv_event.type = Control::EVENT_TYPE_NULL;
                                } else {
                                    mempcpy(((uint8_t*)recv_event.raw_data)+recv_len, data_buffer, overhang_recv_len);
                                }
                                free(data_buffer);
                                recv_len = 0;
                            }
                        }
                        // switch on type
                        switch (recv_event.type) {
                            case Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET: {
                                client_id = recv_event.client_id;
                                MetaGui::logf(log_id, "#I assigned client id %d\n", client_id);
                            } break;
                            default: {
                                MetaGui::logf(log_id, "#I received event, type: %d\n", recv_event.type);
                                recv_queue->push(recv_event);
                            } break;
                        }
                        if (recv_len == 0) {
                            break;
                        }
                    }
                }
            }
        }

        free(data_buffer_base);
        // if the connection is closed, enqueue a connection closed event to ensure the gui resets its connection
        recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
    }
    
}
