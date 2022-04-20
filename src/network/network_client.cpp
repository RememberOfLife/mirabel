#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "SDL_net.h"
#include <openssl/ssl.h>

#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "meta_gui/meta_gui.hpp"
#include "network/protocol.hpp"
#include "network/util.hpp"

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
        ssl_ctx = util_ssl_ctx_init(UTIL_SSL_CTX_TYPE_CLIENT, NULL, NULL);
        if (ssl_ctx == NULL) {
            MetaGui::log(log_id, "#E failed to init ssl ctx\n");
        }
    }

    NetworkClient::~NetworkClient()
    {
        util_ssl_ctx_free(ssl_ctx);
        free(server_address);
        SDLNet_FreeSocketSet(socketset);
        MetaGui::log_unregister(log_id);
    }

    bool NetworkClient::open(const char* host_address, uint16_t host_port)
    {
        if (socketset == NULL || ssl_ctx == NULL) {
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
        SDLNet_TCP_DelSocket(socketset, conn.socket);
        SDLNet_TCP_Close(conn.socket);
        conn.socket = NULL;
        // everything closed, join dead runners
        send_runner.join(); // send_runner joins recv_runner for us
        util_ssl_session_free(&conn);
    }

    void NetworkClient::send_loop()
    {
        // open the socket
        if (SDLNet_ResolveHost(&conn.peer_addr, server_address, server_port)) {
            MetaGui::log(log_id, "#W could not resolve host address\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }
        conn.socket = SDLNet_TCP_Open(&conn.peer_addr);
        if (conn.socket == NULL) {
            MetaGui::log(log_id, "#W socket failed to open\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }
        SDLNet_TCP_AddSocket(socketset, conn.socket); // cant fail, we only have one socket for our size 1 set

        if (!util_ssl_session_init(ssl_ctx, &conn, UTIL_SSL_CTX_TYPE_CLIENT)) {
            MetaGui::log(log_id, "#W ssl session init failed\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }

        // set what the client expects the server hostname cert to match on
        SSL_set_hostflags(conn.ssl_session, 0);
        // ONLY the hostname part, this will match directly against whats defined in the cert, no https or anything
        if (!SSL_set1_host(conn.ssl_session, server_address)) {
            MetaGui::log(log_id, "#W ssl session set verify hostname failed\n");
            recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
            return;
        }

        if (tc) {
            tc_info = tc->register_timeout_item(&send_queue, "networkclient", 1000, 1000);
        }

        // everything fine, enter the actual send loop and start recv_runner
        recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED));
        recv_runner = std::thread(&NetworkClient::recv_loop, this); // socket open, start recv_runner

        size_t base_buffer_size = 8192;
        uint8_t* data_buffer_base = (uint8_t*)malloc(base_buffer_size); // recycled buffer for outgoing data

        // wait until event available
        bool quit = false;
        while (!quit && conn.socket != NULL) {
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
                case Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT: {
                    conn.state = PROTOCOL_CONNECTION_STATE_ACCEPTED;
                    recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT));
                } break;
                default: {
                    if (conn.state != PROTOCOL_CONNECTION_STATE_ACCEPTED) {
                        switch (conn.state) {
                            case PROTOCOL_CONNECTION_STATE_PRECLOSE: {
                                MetaGui::logf(log_id, "#W SECURITY: outgoing event %d on pre-closed connection dropped\n", e.type);
                            } break;
                            case PROTOCOL_CONNECTION_STATE_NONE:
                            case PROTOCOL_CONNECTION_STATE_INITIALIZING: {
                                MetaGui::logf(log_id, "#W SECURITY: outgoing event %d on unsecured connection dropped\n", e.type);
                            } break;
                            default:
                            case PROTOCOL_CONNECTION_STATE_WARNHELD: {
                                MetaGui::logf(log_id, "#W SECURITY: outgoing event %d on unaccepted connection dropped\n", e.type);
                            } break;
                        }
                        break;
                    }
                    // universal event->packet encoding, for POD events
                    uint8_t* data_buffer = data_buffer_base;
                    e.client_id = conn.client_id;
                    int write_len = sizeof(Control::event);
                    if (e.raw_data) {
                        if (e.raw_length > base_buffer_size-write_len) {
                            // if raw data is too big for our reusable buffer, malloc a fitting one
                            data_buffer = (uint8_t*)malloc(write_len+e.raw_length);
                        }
                        memcpy(data_buffer+write_len, e.raw_data, e.raw_length);
                        write_len += e.raw_length;
                    } else {
                        e.raw_length = 0; // security: don't expose side-channel of an unsanitized value
                    }
                    free(e.raw_data);
                    e.raw_data = NULL; // security: don't expose internal pointer to raw data
                    memcpy(data_buffer, &e, sizeof(Control::event));
                    int wrote_len = SSL_write(conn.ssl_session, data_buffer, write_len);
                    if (wrote_len != write_len) {
                        MetaGui::log(log_id, "#W ssl write failed\n");
                    } else {
                        MetaGui::logf(log_id, "wrote event, type %d, len %d\n", e.type, write_len);
                    }
                    if (data_buffer != data_buffer_base) {
                        free(data_buffer);
                    }
                } /* fallthrough */
                case Control::EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE: {
                    // either ssl wants to write, but we dont have anything to send to trigger this ourselves
                    // or fallthrough from event send ssl write, in any case just send forward ssl->tcp
                    while (true) {
                        int pend_len = BIO_ctrl_pending(conn.send_bio);
                        if (pend_len == 0) {
                            // nothing pending to send
                            break;
                        }
                        int send_len = BIO_read(conn.send_bio, data_buffer_base, base_buffer_size);
                        if (send_len == 0) {
                            // empty read, can this happen?
                            break;
                        }
                        int sent_len = SDLNet_TCP_Send(conn.socket, data_buffer_base, send_len);
                        if (sent_len != send_len) {
                            MetaGui::log(log_id, "#W packet sending failed\n");
                        } else {
                            MetaGui::logf(log_id, "sent %d bytes of data\n", sent_len);
                        }
                    }
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
        size_t buffer_size = 8192;
        uint8_t* data_buffer_base = (uint8_t*)malloc(buffer_size); // recycled buffer for incoming data

        while (conn.socket != NULL) {
            int ready = SDLNet_CheckSockets(socketset, 15); //TODO should be UINT32_MAX, but then it doesnt exit on self socket close
            //TODO if we have to go out of waiting anyway every once in a while, the maybe check a dedicated heartbeat inbox here too?
            if (ready == -1) {
                break;
            }
            if (!SDLNet_SocketReady(conn.socket)) {
                continue;
            }
            int recv_len = SDLNet_TCP_Recv(conn.socket, data_buffer_base, buffer_size);
            if (recv_len <= 0) {
                // connection closed
                SDLNet_TCP_DelSocket(socketset, conn.socket);
                SDLNet_TCP_Close(conn.socket);
                conn.socket = NULL;
                switch (conn.state) {
                    case PROTOCOL_CONNECTION_STATE_PRECLOSE: {
                        // pass, everything fine
                    } break;
                    case PROTOCOL_CONNECTION_STATE_NONE: {
                        // refused by server without pre close
                        MetaGui::log(log_id, "#I connection refused without pre-close\n");
                    } break;
                    default:
                    case PROTOCOL_CONNECTION_STATE_INITIALIZING:
                    case PROTOCOL_CONNECTION_STATE_WARNHELD:
                    case PROTOCOL_CONNECTION_STATE_ACCEPTED: {
                        // closed unexpectedly
                        MetaGui::log(log_id, "#W connection closed unexpectedly\n");
                    } break;
                }
                break;
            }

            uint8_t* data_buffer = data_buffer_base;

            if (conn.state == PROTOCOL_CONNECTION_STATE_PRECLOSE) {
                MetaGui::logf(log_id, "#W discarding %d recv bytes on pre-closed connection\n", recv_len);
                continue;
            }

            if (conn.state == PROTOCOL_CONNECTION_STATE_NONE) {
                // we don't know if the server accepted our connection yet, read exactly one event sized packet from the front of the received data
                if (recv_len < sizeof(Control::event)) {
                    //TODO can this happen?
                    MetaGui::logf(log_id, "#W malformed connection initial, discarding %d bytes\n", recv_len);
                    conn.state = PROTOCOL_CONNECTION_STATE_PRECLOSE;
                    continue;
                }
                // decode one raw event packet using the universal packet->event decoding
                Control::event recv_event = Control::event();
                memcpy(&recv_event, data_buffer, sizeof(Control::event));
                // update size of remaining buffer
                data_buffer += sizeof(Control::event);
                recv_len -= sizeof(Control::event);
                // process the event, can only be of two types
                if (recv_event.type == Control::EVENT_TYPE_NETWORK_PROTOCOL_NOK) {
                    MetaGui::log(log_id, "#W connection refused\n");
                    conn.state = PROTOCOL_CONNECTION_STATE_PRECLOSE;
                    continue;
                }
                if (recv_event.type != Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET) {
                    MetaGui::log(log_id, "#W malformed connection initial\n");
                    conn.state = PROTOCOL_CONNECTION_STATE_PRECLOSE;
                    continue;
                }
                conn.client_id = recv_event.client_id;
                MetaGui::logf(log_id, "assigned client id %d\n", conn.client_id);
                conn.state = PROTOCOL_CONNECTION_STATE_INITIALIZING;
                // kick of the handshake from client side and enqueue a want write
                SSL_do_handshake(conn.ssl_session);
                send_queue.push(Control::event(Control::EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE));
                continue;
            }

            // forward tcp->ssl
            // if our buffer is to small, the rest of the data will show up as a ready socket again, then we read it in the next round
            BIO_write(conn.recv_bio, data_buffer, recv_len);
            // if ssl is still doing internal things, don't bother
            if (conn.state == PROTOCOL_CONNECTION_STATE_INITIALIZING) {
                if (!SSL_is_init_finished(conn.ssl_session)) {
                    SSL_do_handshake(conn.ssl_session);
                    // queue generic want write, just in case ssl may want to write
                    send_queue.push(Control::event(Control::EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE));
                    if (!SSL_is_init_finished(conn.ssl_session)) {
                        continue;
                    }
                }
                MetaGui::log(log_id, "ssl connection established\n");
                // handshake is finished, promote connection state if possible
                // SSL peer verification:
                // make sure server presented a certificate
                X509* peer_cert = SSL_get_peer_certificate(conn.ssl_session);
                if (!peer_cert) {
                    conn.state = PROTOCOL_CONNECTION_STATE_PRECLOSE;
                    MetaGui::log(log_id, "#W server did not present certificate, closing connection\n");
                    //TODO send the server a notice that we're disconnecting
                    send_queue.push(Control::event(Control::EVENT_TYPE_EXIT));
                    continue;
                }
                // get result of cert verification
                long verify_result = SSL_get_verify_result(conn.ssl_session);
                conn.state = PROTOCOL_CONNECTION_STATE_WARNHELD; // set to warn per default, overwrite to accepted only on OK
                // print server cert thumbprint in all cases
                //TODO use something shorter? or base 64?
                uint8_t hash_buf[SHA256_LEN];
                unsigned int hash_len = 0;
                int hash_ok = X509_digest(peer_cert, EVP_sha256(), (unsigned char*)hash_buf, &hash_len);
                if (hash_ok == 0 || hash_len != SHA256_LEN) {
                    MetaGui::log(log_id, "server cert (THUMBPRINT FAILURE)\n");
                } else {
                    char str_buf[3*SHA256_LEN]; // size for 2 hex symbols per byte, one separator between each, and the NUL terminator
                    char* str_buf_m = str_buf;
                    for (size_t i = 0; i < SHA256_LEN; i++) {
                        str_buf_m += sprintf(str_buf_m, "%02x", hash_buf[i]);
                        if (i < SHA256_LEN-1) {
                            str_buf_m += sprintf(str_buf_m, ":");
                        }
                    }
                    str_buf[sizeof(str_buf)-1] = '\0';
                    MetaGui::logf(log_id, "server cert thumbprint (%s)\n", str_buf);
                }
                // prepare connection state event for client
                Control::event event_cs = Control::event();
                event_cs.raw_length = SHA256_LEN; // reserve some space for the thumbprint
                switch (verify_result) {
                    case X509_V_OK: {
                        // no verification errors, promote to accepted
                        conn.state = PROTOCOL_CONNECTION_STATE_ACCEPTED;
                        MetaGui::log(log_id, "server cert verification passed\n");
                        event_cs.type = Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT;
                        event_cs.raw_data = malloc(event_cs.raw_length);
                    } break;
                    case X509_V_ERR_CERT_HAS_EXPIRED: {
                        BIO* print_bio = BIO_new(BIO_s_mem()); // bio for printing details about the failure
                        const ASN1_TIME* exp_date = X509_get0_notAfter(peer_cert);
                        ASN1_TIME_print(print_bio, exp_date); // only offers the default format
                        //TODO need ASN1_STRING_free(exp_date) here?
                        size_t time_str_len = BIO_ctrl_pending(print_bio);
                        char* time_str = (char*)malloc(time_str_len);
                        BIO_read(print_bio, time_str, time_str_len);
                        MetaGui::logf(log_id, "#W server cert verification failed: cert has expired (%s)\n", time_str);
                        const char* err_str = "expired (%s)";
                        event_cs.type = Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL;
                        event_cs.raw_length += strlen(err_str) + 1 + time_str_len;
                        event_cs.raw_data = malloc(event_cs.raw_length);
                        sprintf((char*)event_cs.raw_data+SHA256_LEN, err_str, time_str);
                        free(time_str);
                        BIO_free(print_bio);
                    } break;
                    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT: {
                        MetaGui::log(log_id, "#W server cert verification failed: depth zero self signed cert\n");
                        const char* err_str = "depth zero self signed";
                        event_cs.type = Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL;
                        event_cs.raw_length += strlen(err_str) + 1;
                        event_cs.raw_data = malloc(event_cs.raw_length);
                        strcpy((char*)event_cs.raw_data+SHA256_LEN, err_str);
                    } break;
                    case X509_V_ERR_HOSTNAME_MISMATCH: {
                        char** name_list;
                        int name_count;
                        size_t names_totalsize = util_cert_get_subjects(peer_cert, &name_list, &name_count);
                        char* str_buf = (char*)malloc(names_totalsize + name_count*2 - 1); // need extra space for ", "
                        char* str_buf_m = str_buf;
                        for (int i = 0; i < name_count; i++) {
                            strcpy(str_buf_m, name_list[i]);
                            str_buf_m += strlen(name_list[i]);
                            if (i < name_count - 1) {
                                // add comma separator
                                strcpy(str_buf_m, ", ");
                                str_buf_m += 2;
                            }
                        }
                        *str_buf_m = '\0';
                        MetaGui::logf(log_id, "#W server cert verification failed: hostname mismatch (%s)\n", str_buf);
                        const char* err_str = "hostname mismatch (%s)";
                        event_cs.type = Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL;
                        event_cs.raw_length += strlen(err_str) + 1 + (str_buf_m - str_buf);
                        event_cs.raw_data = malloc(event_cs.raw_length);
                        sprintf((char*)event_cs.raw_data+SHA256_LEN, err_str, str_buf);
                        free(str_buf);
                        util_cert_free_subjects(name_list, name_count);
                    } break;
                    default: {
                        MetaGui::logf(log_id, "#W server cert verification failed: x509 v err %lu\n", verify_result);
                        const char* err_str = "x509 v err %lu";
                        event_cs.type = Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL;
                        event_cs.raw_length += strlen(err_str) + 1 + 30; //TODO replace 30 by proper %lu size
                        event_cs.raw_data = malloc(event_cs.raw_length);
                        sprintf((char*)event_cs.raw_data+SHA256_LEN, err_str, verify_result);
                    } break;
                }
                X509_free(peer_cert);
                // expiry, dz self signed, hostname mismatch all push an adapter event so the connection window offers a button for the user to accept the connection anyway, if everything is fine we push the connection accept instead
                memcpy((char*)event_cs.raw_data, hash_buf, SHA256_LEN); // place thumbprint before the reason string
                recv_queue->push(event_cs);
            }

            if (conn.state == PROTOCOL_CONNECTION_STATE_WARNHELD) {
                //TODO should at least keep track of the byte size of the accumulated buffer
                continue;
            }

            data_buffer = data_buffer_base;
            recv_len = 0;
            while (recv_len < buffer_size) {
                // need to do multiple reads, even if pending is 0, because every ssl read will always only output content from ONE corresponding ssl write
                int im_rd = SSL_read(conn.ssl_session, data_buffer+recv_len, buffer_size-recv_len); // read as much from ssl as we can to jumpstart event processing
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
                        int overhang_recv_len = SSL_read(conn.ssl_session, data_buffer, raw_overhang);
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
                    case Control::EVENT_TYPE_NULL: break; // drop null events
                    case Control::EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET: {
                        conn.client_id = recv_event.client_id;
                        MetaGui::logf(log_id, "#I re-assigned client id %d\n", conn.client_id);
                    } break;
                    case Control::EVENT_TYPE_NETWORK_PROTOCOL_PONG: {
                        MetaGui::log(log_id, "received pong\n");
                    } break;
                    default: {
                        // general purpose events get pushed to the recv queue
                        MetaGui::logf(log_id, "received event, type: %d\n", recv_event.type);
                        recv_queue->push(recv_event);
                    } break;
                }
                if (recv_len == 0) {
                    break;
                }
            }

            // loop into next wait on socketset
        }

        free(data_buffer_base);
        // if the connection is closed, enqueue a connection closed event to ensure the gui resets its connection
        recv_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED));
    }
    
}
