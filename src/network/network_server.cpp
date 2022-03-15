#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "SDL_net.h"

#include "control/event_queue.hpp"
#include "control/event.hpp"

#include "network/network_server.hpp"

namespace Network {

    NetworkServer::NetworkServer()
    {
        server_socketset = SDLNet_AllocSocketSet(1);
        if (server_socketset == NULL) {
            printf("[ERROR] failed to allocate server socketset\n");
        }
        client_connections = (client_connection*)malloc(client_connection_bucket_size*sizeof(client_connection));
        if (client_connections == NULL) {
            printf("[ERROR] failed to allocate client connections bucket\n");
        }
        for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
            client_connections[i].socket = NULL;
            client_connections[i].client_id = 0;
        }
        client_socketset = SDLNet_AllocSocketSet(client_connection_bucket_size);
        if (client_socketset == NULL) {
            printf("[ERROR] failed to allocate client socketset\n");
        }
    }

    NetworkServer::~NetworkServer()
    {
        SDLNet_FreeSocketSet(client_socketset);
        free(client_connections);
        SDLNet_FreeSocketSet(server_socketset);
    }

    bool NetworkServer::open(const char* host_address, uint16_t host_port)
    {
        if (server_socketset == NULL || client_connections == NULL || client_socketset == NULL) {
            printf("[ERROR] server construction failure\n");
            return false;
        }
        if (SDLNet_ResolveHost(&server_ip, NULL, host_port) != 0) {
            printf("[ERROR] could not resolve server address\n");
            return false;
        }
        server_socket = SDLNet_TCP_Open(&server_ip);
        if (server_socket == NULL) {
            printf("[ERROR] server socket failed to open\n");
            return false;
        }
        SDLNet_TCP_AddSocket(server_socketset, server_socket); // cant fail, we only have one socket for our size 1 set
        server_runner = std::thread(&NetworkServer::server_loop, this); // socket open, start server_runner
        send_runner = std::thread(&NetworkServer::send_loop, this); // socket open, start send_runner
        recv_runner = std::thread(&NetworkServer::recv_loop, this); // socket open, start recv_runner
        return true;
    }

    void NetworkServer::close()
    {
        // stop server_runner
        SDLNet_TCP_DelSocket(server_socketset, server_socket);
        SDLNet_TCP_Close(server_socket);
        server_socket = NULL;
        send_queue.push(Control::event(Control::EVENT_TYPE_EXIT)); // stop send_runner
        // stop recv_runner
        for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
            TCPsocket* client_socket = &(client_connections[i].socket);
            SDLNet_TCP_DelSocket(client_socketset, *client_socket);
            SDLNet_TCP_Close(*client_socket);
            *client_socket = NULL;
        }
        // everything closed, join dead runners
        server_runner.join();
        send_runner.join();
        recv_runner.join();
    }

    void NetworkServer::server_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer = (uint8_t*)malloc(buffer_size); // recycled buffer for data
        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer);

        while (server_socket != NULL) {
            int ready = SDLNet_CheckSockets(server_socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            if (ready == -1) {
                break;
            }
            // handle new connection
            if (SDLNet_SocketReady(server_socket)) {
                TCPsocket incoming_socket;
                incoming_socket = SDLNet_TCP_Accept(server_socket);
                if (incoming_socket == NULL) {
                    printf("[ERROR] server socket closed unexpectedly\n");
                    break;
                }
                // check if there is still space for a new client connection
                //TODO link clients into a list so this is O(1), also generally keep counter of clients per bucket
                client_connection* connection_slot = NULL;
                uint32_t connection_id = 0;
                for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                    if (client_connections[i].socket == NULL) {
                        connection_slot = &(client_connections[i]);
                        connection_id = i+1;
                        break;
                    }
                }
                if (connection_slot == NULL) {
                    // no slot available for new client connection, drop it
                    *db_event_type = Control::EVENT_TYPE_NETWORK_PROTOCOL_NOK;
                    int send_len = sizeof(Control::event);
                    int sent_len = SDLNet_TCP_Send(incoming_socket, data_buffer, sizeof(Control::event));
                    if (sent_len != send_len) {
                        printf("[WARN] packet sending failed\n");
                    }
                    SDLNet_TCP_Close(incoming_socket);
                    printf("[INFO] refused new connection\n");
                } else {
                    // slot available for new client, accept it
                    // send protocol ok
                    *db_event_type = Control::EVENT_TYPE_NETWORK_PROTOCOL_OK;
                    *(db_event_type+1) = 0;
                    int send_len = sizeof(Control::event);
                    int sent_len = SDLNet_TCP_Send(incoming_socket, data_buffer, sizeof(Control::event));
                    if (sent_len != send_len) {
                        printf("[WARN] packet sending failed\n");
                    }
                    *db_event_type = Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET;
                    *(db_event_type+1) = connection_id;
                    sent_len = SDLNet_TCP_Send(incoming_socket, data_buffer, sizeof(Control::event));
                    if (sent_len != send_len) {
                        printf("[WARN] packet sending failed\n");
                    }
                    connection_slot->socket = incoming_socket;
                    connection_slot->peer = *SDLNet_TCP_GetPeerAddress(incoming_socket);
                    connection_slot->client_id = connection_id;
                    SDLNet_TCP_AddSocket(client_socketset, connection_slot->socket);
                    printf("[INFO] accepted new connection, client id %d\n", connection_id);
                }
            }
        }

        free(data_buffer);
        //TODO does anyone need to know that the server_loop closed?
    }

    void NetworkServer::send_loop()
    {
        uint32_t base_buffer_size = 1024;
        uint8_t* data_buffer_base = (uint8_t*)malloc(base_buffer_size); // recycled buffer for outgoing data

        // wait until event available
        bool quit = false;
        while (!quit) {
            Control::event e = send_queue.pop(UINT32_MAX);
            switch (e.type) {
                case Control::EVENT_TYPE_NULL: {
                    printf("[WARN] received impossible null event\n");
                } break;
                case Control::EVENT_TYPE_EXIT: {
                    quit = true;
                    break;
                } break;
                //TODO heartbeat
                default: {
                    // find target client connection to send to
                    client_connection* target_client = NULL;
                    for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                        if (client_connections[i].client_id == e.client_id) {
                            target_client = &(client_connections[i]);
                            break;
                        }
                    }
                    if (target_client == NULL) {
                        printf("[WARN] failed to find connection for sending event, discarded %lu bytes\n",
                            sizeof(Control::event) + ((e.raw_data) ? e.raw_length : 0));
                        break;
                    }
                    // universal event->packet encoding, for POD events
                    uint8_t* data_buffer = data_buffer_base;
                    e.client_id = e.client_id;
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
                    int sent_len = SDLNet_TCP_Send(target_client->socket, data_buffer, send_len);
                    if (sent_len != send_len) {
                        printf("[WARN] packet sending failed\n");
                    }
                    if (data_buffer != data_buffer_base) {
                        free(data_buffer);
                    }
                    printf("[INFO] sent event to client id %d, type %d, len %d\n", e.client_id, e.type, send_len);
                } break;
            }
        }

        free(data_buffer_base);
    }

    void NetworkServer::recv_loop()
    {
        uint32_t buffer_size = 1024;
        uint8_t* data_buffer_base = (uint8_t*)malloc(buffer_size); // recycled buffer for incoming data
        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer_base);

        while (true) {
            int ready = SDLNet_CheckSockets(client_socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            if (ready == -1) {
                break;
            }
            client_connection* ready_client = NULL;
            for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                if (!SDLNet_SocketReady(client_connections[i].socket)) {
                    continue;
                }
                ready_client = &(client_connections[i]);
                // handle data for the ready_client
                int recv_len = SDLNet_TCP_Recv(ready_client->socket, data_buffer_base, buffer_size);
                if (recv_len <= 0) {
                    // connection closed
                    SDLNet_TCP_DelSocket(client_socketset, ready_client->socket);
                    SDLNet_TCP_Close(ready_client->socket);
                    printf("[WARN] client id %d connection closed unexpectedly\n", ready_client->client_id);
                    ready_client->socket = NULL;
                    ready_client->client_id = 0;
                } else {
                    // one call to recv may receive MULTIPLE events at once, process them all
                    uint8_t* data_buffer = data_buffer_base;
                    while (true) {
                        if (recv_len < sizeof(Control::event)) {
                            printf("[WARN] discarding %d unusable bytes of received data\n", recv_len);
                            break;
                        }
                        // universal packet->event decoding, then place it in the recv_queue
                        // at least one event here, process it from data_buffer
                        Control::event recv_event = Control::event();
                        memcpy(&recv_event, data_buffer, sizeof(Control::event));
                        if (recv_event.client_id != ready_client->client_id) {
                            printf("[WARN] client id %d provided wrong id %d in incoming packet\n", ready_client->client_id, recv_event.client_id);
                            recv_event.client_id = ready_client->client_id;
                        }
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
                                int overhang_recv_len = SDLNet_TCP_Recv(ready_client->socket, data_buffer, raw_overhang);
                                if (overhang_recv_len != raw_overhang) {
                                    // did not receive all the missing bytes, might be disastrous for the event
                                    printf("[ERROR] received only %d bytes of overhang, expected %d, event might be corrupted\n", overhang_recv_len, raw_overhang);
                                    //TODO drop event to null type?
                                    memset(((uint8_t*)recv_event.raw_data)+recv_len+overhang_recv_len,
                                        0x00, recv_event.raw_length-(recv_len+overhang_recv_len)); // at least zero out any left over bytes
                                }
                                mempcpy(((uint8_t*)recv_event.raw_data)+recv_len, data_buffer, overhang_recv_len);
                                free(data_buffer);
                                recv_len = 0;
                            }
                        }
                        // switch on type
                        switch (recv_event.type) {
                            case Control::EVENT_TYPE_NETWORK_PROTOCOL_PING: {
                                printf("[INFO] ping from client sending pong\n");
                                send_queue.push(Control::event(Control::EVENT_TYPE_NETWORK_PROTOCOL_PONG, recv_event.client_id));
                            } break;
                            default: {
                                printf("[INFO] received event from client id %d, type: %d\n", ready_client->client_id, recv_event.type);
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
        //TODO does anyone need to know that the recv_loop closed?
    }

}
