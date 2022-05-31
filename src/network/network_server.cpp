#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "SDL_net.h"
#include <openssl/ssl.h>

#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "network/util.hpp"

#include "network/network_server.hpp"

namespace Network {

    NetworkServer::NetworkServer()
    {
        server_socketset = SDLNet_AllocSocketSet(1);
        if (server_socketset == NULL) {
            printf("[ERROR] failed to allocate server socketset\n");
        }
        client_connections = (connection*)malloc(client_connection_bucket_size*sizeof(connection));
        if (client_connections == NULL) {
            printf("[ERROR] failed to allocate client connections bucket\n");
        }
        for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
            client_connections[i] = connection();
        }
        client_socketset = SDLNet_AllocSocketSet(client_connection_bucket_size);
        if (client_socketset == NULL) {
            printf("[ERROR] failed to allocate client socketset\n");
        }
        ssl_ctx = util_ssl_ctx_init(UTIL_SSL_CTX_TYPE_SERVER, "./server-fullchain.pem", "./server-privkey.pem"); //TODO dont hardcode cert names
        if (ssl_ctx == NULL) {
            printf("[ERROR] failed to init ssl ctx\n");
        }
    }

    NetworkServer::~NetworkServer()
    {
        util_ssl_ctx_free(ssl_ctx);
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
        send_queue.push(Control::f_event(Control::EVENT_TYPE_EXIT)); // stop send_runner
        // stop recv_runner
        for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
            TCPsocket* client_socket = &(client_connections[i].socket);
            SDLNet_TCP_DelSocket(client_socketset, *client_socket);
            SDLNet_TCP_Close(*client_socket);
            *client_socket = NULL;
            util_ssl_session_free(&(client_connections[i]));
        }
        // everything closed, join dead runners
        server_runner.join();
        send_runner.join();
        recv_runner.join();
    }

    void NetworkServer::server_loop()
    {
        uint32_t buffer_size = 8192;
        uint8_t* data_buffer = (uint8_t*)malloc(buffer_size); // recycled buffer for data
        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer);

        while (server_socket != NULL) {
            int ready = SDLNet_CheckSockets(server_socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            //TODO if we have to go out of waiting anyway every once in a while, the maybe check a dedicated heartbeat inbox here too?
            if (ready == -1) {
                break;
            }
            // handle new connection
            if (!SDLNet_SocketReady(server_socket)) {
                continue;
            }
            TCPsocket incoming_socket;
            incoming_socket = SDLNet_TCP_Accept(server_socket);
            if (incoming_socket == NULL) {
                printf("[ERROR] server socket closed unexpectedly\n");
                break;
            }
            // check if there is still space for a new client connection
            connection* connection_slot = NULL;
            uint32_t connection_id = Control::CLIENT_NONE;
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
                int send_len = sizeof(Control::f_event);
                int sent_len = SDLNet_TCP_Send(incoming_socket, data_buffer, sizeof(Control::f_event));
                if (sent_len != send_len) {
                    printf("[WARN] packet sending failed\n");
                }
                SDLNet_TCP_Close(incoming_socket);
                printf("[INFO] refused new connection\n");
            } else {
                // slot available for new client, accept it
                // send protocol client id set, functions as ok if set as initial
                *db_event_type = Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET;
                *(db_event_type+1) = connection_id;
                int send_len = sizeof(Control::f_event);
                int sent_len = SDLNet_TCP_Send(incoming_socket, data_buffer, sizeof(Control::f_event));
                if (sent_len != send_len) {
                    printf("[WARN] packet sending failed\n");
                }
                connection_slot->state = PROTOCOL_CONNECTION_STATE_NONE;
                connection_slot->socket = incoming_socket;
                connection_slot->peer_addr = *SDLNet_TCP_GetPeerAddress(incoming_socket);
                connection_slot->client_id = connection_id;
                util_ssl_session_init(ssl_ctx, connection_slot, UTIL_SSL_CTX_TYPE_SERVER);
                SDLNet_TCP_AddSocket(client_socketset, connection_slot->socket);
                printf("[INFO] new connection initializing, client id %d\n", connection_id);
            }
        }

        free(data_buffer);
        // if server_loop closes, notify server so it can handle it
        recv_queue->push(Control::f_event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
    }

    void NetworkServer::send_loop()
    {
        uint32_t base_buffer_size = 8192;
        uint8_t* data_buffer_base = (uint8_t*)malloc(base_buffer_size); // recycled buffer for outgoing data

        // wait until event available
        bool quit = false;
        while (!quit) {
            Control::f_any_event e = send_queue.pop(UINT32_MAX);
            connection* target_client = NULL; // target client to use for processing this event
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
                    //TODO use client id as index into the bucket, give every bucket a base offset
                    for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                        if (client_connections[i].client_id == e.client_id) {
                            target_client = &(client_connections[i]);
                            break;
                        }
                    }
                    if (target_client == NULL) {
                        printf("[WARN] failed to find connection for sending event, discarded %lu bytes\n", Control::event_size(&e));
                        break;
                    }
                    if (target_client->state != PROTOCOL_CONNECTION_STATE_ACCEPTED) {
                        switch (target_client->state) {
                            case PROTOCOL_CONNECTION_STATE_PRECLOSE: {
                                printf("[WARN] SECURITY: outgoing event %d on pre-closed connection dropped\n", e.type);
                            } break;
                            case PROTOCOL_CONNECTION_STATE_NONE:
                            case PROTOCOL_CONNECTION_STATE_INITIALIZING: {
                                printf("[WARN] SECURITY: outgoing event %d on unsecured connection dropped\n", e.type);
                            } break;
                            default:
                            case PROTOCOL_CONNECTION_STATE_WARNHELD: {
                                printf("[WARN] SECURITY: outgoing event %d on unaccepted connection dropped\n", e.type);
                            } break;
                        }
                        break;
                    }
                    // universal event->packet encoding, for POD events
                    uint8_t* data_buffer = data_buffer_base;
                    int write_len = Control::event_size(&e);
                    if (write_len > base_buffer_size) {
                        data_buffer = (uint8_t*)malloc(write_len);
                    }

                    Control::event_serialize(&e, data_buffer);
                    int wrote_len = SSL_write(target_client->ssl_session, data_buffer, write_len);
                    if (wrote_len != write_len) {
                        printf("[WARN] ssl write failed\n");
                    } else {
                        printf("[----] wrote event, type %d, len %d\n", e.type, write_len);
                    }
                    if (data_buffer != data_buffer_base) {
                        free(data_buffer);
                    }
                } /* fallthrough */
                case Control::EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE: {
                    // either ssl wants to write, but we dont have anything to send to trigger this ourselves
                    // or fallthrough from event send ssl write, in any case just send forward ssl->tcp

                    // if this is not a fallthrough we need to find the target client
                    if (target_client == NULL) {
                        // find target client connection to send to
                        //TODO use client id as index into the bucket, give every bucket a base offset
                        for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                            if (client_connections[i].client_id == e.client_id) {
                                target_client = &(client_connections[i]);
                                break;
                            }
                        }
                        if (target_client == NULL) {
                            printf("[WARN] failed to find connection %d for sending ssl write\n", e.client_id);
                            break;
                        }
                    }

                    while (true) {
                        int pend_len = BIO_ctrl_pending(target_client->send_bio);
                        if (pend_len == 0) {
                            // nothing pending to send
                            break;
                        }
                        int send_len = BIO_read(target_client->send_bio, data_buffer_base, base_buffer_size);
                        if (send_len == 0) {
                            // empty read, can this happen?
                            break;
                        }
                        int sent_len = SDLNet_TCP_Send(target_client->socket, data_buffer_base, send_len);
                        if (sent_len != send_len) {
                            printf("[WARN] packet sending failed\n");
                        } else {
                            printf("[----] sent %d bytes of data to client id %d\n", sent_len, e.client_id);
                        }
                    }
                } break;
            }
        }

        free(data_buffer_base);
    }

    void NetworkServer::recv_loop()
    {
        uint32_t buffer_size = 8192;
        uint8_t* data_buffer_base = (uint8_t*)malloc(buffer_size); // recycled buffer for incoming data
        uint32_t* db_event_type = reinterpret_cast<uint32_t*>(data_buffer_base);

        while (true) {
            int ready = SDLNet_CheckSockets(client_socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            //TODO if we have to go out of waiting anyway every once in a while, the maybe check a dedicated heartbeat inbox here too?
            if (ready == -1) {
                break;
            }
            connection* ready_client = NULL;
            for (uint32_t i = 0; i < client_connection_bucket_size; i++) {
                //TODO traverse clients in order of activity
                if (ready <= 0) {
                    break; // exit search for ready clients early if we already served the ready count
                }
                if (!SDLNet_SocketReady(client_connections[i].socket)) {
                    continue;
                }
                ready--;
                ready_client = &(client_connections[i]);
                // handle data for the ready_client
                int recv_len = SDLNet_TCP_Recv(ready_client->socket, data_buffer_base, buffer_size);
                if (recv_len <= 0) {
                    // connection closed
                    SDLNet_TCP_DelSocket(client_socketset, ready_client->socket);
                    SDLNet_TCP_Close(ready_client->socket);
                    switch (ready_client->state) {
                        default:
                        case PROTOCOL_CONNECTION_STATE_NONE: {
                            printf("[WARN] client id %d connection closed before initialization\n", ready_client->client_id);
                        } break;
                        case PROTOCOL_CONNECTION_STATE_INITIALIZING: {
                            printf("[WARN] client id %d connection closed while initializing\n", ready_client->client_id);
                        } break;
                        case PROTOCOL_CONNECTION_STATE_WARNHELD:
                        case PROTOCOL_CONNECTION_STATE_ACCEPTED: // both closed unexpectedly
                        case PROTOCOL_CONNECTION_STATE_PRECLOSE: {
                            // pass, everything fine
                            if (ready_client->state == PROTOCOL_CONNECTION_STATE_PRECLOSE) {
                                printf("[INFO] client id %d connection closed\n", ready_client->client_id);
                            } else {
                                printf("[WARN] client id %d connection closed unexpectedly\n", ready_client->client_id);
                            }
                            recv_queue->push(Control::f_event(Control::EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED, ready_client->client_id));
                        } break;
                    }
                    util_ssl_session_free(ready_client);
                    ready_client->reset(); // sets everything 0/NULL/NONE
                    continue;
                }

                uint8_t* data_buffer = data_buffer_base;

                if (ready_client->state == PROTOCOL_CONNECTION_STATE_PRECLOSE) {
                    printf("[WARN] discarding %d recv bytes on pre-closed connection\n", recv_len);
                    continue;
                }

                if (ready_client->state == PROTOCOL_CONNECTION_STATE_NONE) {
                    // first client response after connection established
                    ready_client->state = PROTOCOL_CONNECTION_STATE_INITIALIZING;
                }

                // forward tcp->ssl
                // if our buffer is to small, the rest of the data will show up as a ready socket again, then we read it in the next round
                BIO_write(ready_client->recv_bio, data_buffer, recv_len);
                // if ssl is still doing internal things, don't bother
                if (ready_client->state == PROTOCOL_CONNECTION_STATE_INITIALIZING) {
                    if (!SSL_is_init_finished(ready_client->ssl_session)) {
                        SSL_do_handshake(ready_client->ssl_session);
                        // queue generic want write, just in case ssl may want to write
                        send_queue.push(Control::f_event(Control::EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE, ready_client->client_id));
                        if (!SSL_is_init_finished(ready_client->ssl_session)) {
                            continue;
                        }
                    }
                    // handshake is finished, promote connection state if possible
                    // no verification necessary on server side
                    ready_client->state = PROTOCOL_CONNECTION_STATE_ACCEPTED;
                    printf("[INFO] client %d connection accepted\n", ready_client->client_id);
                    //REWORK this never reaches the client at the right point in time, it is sent before the adapter is installed
                    // somehow make sure we only USE the client when has authenticated, i.e. installed its adapter
                    recv_queue->push(Control::f_event(Control::EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED, ready_client->client_id)); // inform server that client is connected and ready to use
                }

                // PROTOCOL_CONNECTION_STATE_WARNHELD, server never uses this

                data_buffer = data_buffer_base;
                recv_len = 0;
                while (recv_len < buffer_size) {
                    // need to do multiple reads, even if pending is 0, because every ssl read will always only output content from ONE corresponding ssl write
                    int im_rd = SSL_read(ready_client->ssl_session, data_buffer+recv_len, buffer_size-recv_len); // read as much from ssl as we can to jumpstart event processing
                    if (im_rd == 0) {
                        break;
                    }
                    recv_len += im_rd;
                }
                if (recv_len == 0) {
                    // empty ssl read, don't do processing
                    continue;
                }

                // one call to recv may receive MULTIPLE events at once, process them all
                while (true) {

                    if (recv_len < sizeof(size_t)) {
                        printf("[WARN] %d invalid packet length bytes received\n", recv_len);
                        break;
                    }
                    size_t event_length = *(size_t*)data_buffer;
                    if (recv_len < event_length) {
                        //TODO this might also be the case when the data received is simply larger than the buffer for it
                        printf("[WARN] discarding %d unusable bytes of received data\n", recv_len);
                        break;
                    }
                    //TODO handle events longer than one buffer filling
                    //TODO handle event fragmentation
                    
                    // universal packet->event decoding, then place it in the recv_queue
                    // at least one event here, process it from data_buffer
                    Control::f_any_event recv_event = Control::f_any_event();
                    Control::event_deserialize(&recv_event, data_buffer, (char*)data_buffer + event_length);

                    if (recv_event.client_id != ready_client->client_id) {
                        printf("[WARN] client id %d provided wrong id %d in incoming packet\n", ready_client->client_id, recv_event.client_id);
                        recv_event.client_id = ready_client->client_id;
                    }

                    // update size of remaining buffer
                    data_buffer += event_length;
                    recv_len -= event_length;

                    // switch on type
                    switch (recv_event.type) {
                        case Control::EVENT_TYPE_NULL: break; // drop null events
                        case Control::EVENT_TYPE_NETWORK_PROTOCOL_DISCONNECT: {
                            //REWORK need more?
                            ready_client->state = PROTOCOL_CONNECTION_STATE_PRECLOSE;
                        } break;
                        case Control::EVENT_TYPE_NETWORK_PROTOCOL_PING: {
                            printf("[INFO] ping from client sending pong\n");
                            send_queue.push(Control::f_event(Control::EVENT_TYPE_NETWORK_PROTOCOL_PONG, recv_event.client_id));
                        } break;
                        default: {
                            printf("[----] received event from client id %d, type: %d\n", ready_client->client_id, recv_event.type);
                            recv_queue->push(recv_event);
                        } break;
                    }
                    if (recv_len == 0) {
                        break;
                    }
                }

                // loop into ready check on next client connection
            }

            // loop into next wait on socketset
        }

        free(data_buffer_base);
        // if server_loop closes, notify server so it can handle it
        recv_queue->push(Control::f_event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
    }

}
