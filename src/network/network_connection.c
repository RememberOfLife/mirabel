#include <stdbool.h>

#include "rosalia/serialization.h"

#include "mirabel/alloc.h"
#include "mirabel/event_queue.h"
#include "mirabel/log.h"
#include "mirabel/network_adapter.h"
#include "mirabel/network_connection.h"

/////
//public

const char* default_adapter_server_address = "run.mirabel.dev";
const uint16_t default_adapter_server_port = 61801;

bool network_connection_create(network_connection* self)
{
    self->deleted = false;

    self->adapter_server_address[0] = '\0';
    self->adapter_server_port = default_adapter_server_port;
    self->adapter_state = RSI_IDLE;
    self->adapter.inbox = &self->inbox;
    self->adapter.methods = NULL;
    self->adapter_error = NULL;

    self->connection_state = RSI_IDLE;
    self->connection_cert_thumb = BLOB_NULL;
    self->connection_verifail_reason = NULL;

    self->authinfo_state = RSI_NONE;

    self->authn_username[0] = '\0';
    self->authn_password[0] = '\0';
    self->authn_state = RSI_IDLE;
    self->authn_fail_reason = NULL;

    self->outbox = &self->adapter.outbox;
    event_queue_create(&self->inbox);

    // VEC_CREATE(&self->connected_workspace_idcs, 0);

    return false;
}

void network_connection_destroy(network_connection* self)
{
    //TODO

    // VEC_DESTROY(&self->connected_workspace_idcs);

    event_queue_destroy(&self->inbox);

    if (self->authn_fail_reason != NULL) {
        mirabel_free(self->authn_fail_reason);
    }

    if (self->connection_verifail_reason != NULL) {
        mirabel_free(self->connection_verifail_reason);
    }
}

void network_connection_outbox_push(network_connection* self, event_any* e)
{
    bool consumed = false;
    switch (e->base.type) {
        case EVENT_TYPE_NETWORK_PROTOCOL_PING: {
            mirabel_slogf(LOGS_OK, "sending ping #%u", e->base.association_id);
        } break;
        case EVENT_TYPE_NETWORK_ADAPTER_OPEN: {
            self->adapter_state = RSI_WAITING;
            if (self->adapter_error != NULL) {
                mirabel_free(self->adapter_error);
                self->adapter_error = NULL;
            }
            network_adapter_create(&self->adapter);
        } break;
        case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
            //TODO this should be instantly reflected in the state, and not just once the adapter gives it back, need to keep a direction for the connection state, i.e. e.g. bool closing_not_opening, then we can use the connection_state:WAITING for waiting for a disconnect..
        } break;
        case EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT: {
            // pass
        } break;
        case EVENT_TYPE_USER_AUTH_INFO: {
            self->authn_state = RSI_WAITING;
        } break;
        case EVENT_TYPE_USER_AUTH_REJECT: {
            self->authinfo_state = RSI_WAITING;
            self->authn_state = RSI_IDLE;
        } break;
        //TODO our relevant cases..
        default: {
            // pass
        } break;
    }
    if (consumed) {
        event_destroy(e);
    } else {
        event_queue_push(self->outbox, e);
    }
}

void network_connection_inbox_pop(network_connection* self, event_any* e)
{
    bool consumed = true;
    while (consumed) {
        event_queue_pop(&self->inbox, e, 0);
        switch (e->base.type) {
            case EVENT_TYPE_NETWORK_PROTOCOL_PONG: {
                mirabel_slogf(LOGS_OK, "received pong #%u", e->base.association_id);
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_OPEN: {
                self->adapter_state = RSI_DONE;
                if (self->connection_verifail_reason != NULL) {
                    mirabel_free(self->connection_verifail_reason);
                    self->connection_verifail_reason = NULL;
                }
                self->connection_state = RSI_WAITING;
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                self->adapter_state = RSI_IDLE;
                self->connection_state = RSI_IDLE;
                self->authinfo_state = RSI_IDLE;
                self->authn_state = RSI_IDLE;
                network_adapter_destroy(&self->adapter);
                mirabel_slogf(LOGS_NORM, "connection closed: %p %s %hu", self, e->neta_open.server_addr, e->neta_open.server_port); //REMOVE
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT: {
                blob_move(&self->connection_cert_thumb, &e->neta_veri.thumb);
                self->connection_verifail_reason = e->neta_veri.reason;
                e->neta_veri.reason = NULL;
                self->connection_state = RSI_DONE;
                self->authinfo_state = RSI_WAITING;
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT: {
                blob_move(&self->connection_cert_thumb, &e->neta_veri.thumb);
                self->connection_verifail_reason = e->neta_veri.reason;
                e->neta_veri.reason = NULL;
            } break;
            case EVENT_TYPE_USER_AUTH_INFO: {
                if (self->authn_state == RSI_IDLE) {
                    // if is_guest is true the server accepts guest logins, otherwise not
                    self->authinfo_allow_guest = e->user_auth_info.is_guest;
                    // if username is NULL the server does NOT accept user logins
                    self->authinfo_allow_login = (e->user_auth_info.username != NULL);
                    // if password is NULL the server does NOT require a server password for guests
                    self->authinfo_want_guest_pw = (e->user_auth_info.password != NULL);
                    // if the server does not accept user AND guest logins wait for user to press guest login, enable pw input if wanted
                    self->authinfo_state = RSI_DONE;
                    self->authn_state = RSI_IDLE;
                    if (self->authn_fail_reason != NULL) {
                        mirabel_free(self->authn_fail_reason);
                        self->authn_fail_reason = NULL;
                    }
                    self->authn_username[0] = '\0';
                    self->authn_password[0] = '\0';
                } else {
                    // we received our login
                    if (e->user_auth_info.username != NULL) {
                        strncpy(self->authn_username, e->user_auth_info.username, CONNECTION_AUTHN_USERNAME_SIZE);
                    }
                    if (e->user_auth_info.password != NULL) {
                        strncpy(self->authn_password, e->user_auth_info.password, CONNECTION_AUTHN_PASSWORD_SIZE);
                    }
                    if (self->authn_fail_reason != NULL) {
                        mirabel_free(self->authn_fail_reason);
                        self->authn_fail_reason = NULL;
                    }
                    self->authn_state = RSI_DONE;
                }
            } break;
            case EVENT_TYPE_USER_AUTH_REJECT: {
                self->authn_fail_reason = e->user_auth_reject.reason;
                e->user_auth_reject.reason = NULL;
                self->authn_state = RSI_IDLE;
            } break;
            //TODO our relevant cases..
            default: {
                consumed = false;
            } break;
        }
        if (consumed) {
            event_destroy(e);
        }
    }
}

void network_connection_adapter_open(network_connection* self)
{
    event_any e;
    event_create_neta_open(&e, self->adapter_server_address, self->adapter_server_port);
    network_connection_outbox_push(self, &e);
}

void network_connection_adapter_close(network_connection* self)
{
    event_any e;
    event_create_neta_close(&e, NULL); //TODO reason? "user disconnected"
    network_connection_outbox_push(self, &e);
}

void network_connection_ping(network_connection* self)
{
    event_any e;
    event_create_type_association(&e, EVENT_TYPE_NETWORK_PROTOCOL_PING, get_new_association_id());
    network_connection_outbox_push(self, &e);
}

void network_connection_veriaccept(network_connection* self)
{
    event_any e;
    event_create_neta_veri(&e, EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT, BLOB_NULL, NULL);
    network_connection_outbox_push(self, &e);
}

void network_connection_authn_login(network_connection* self, bool guest_not_user)
{
    event_any e;
    event_create_user_auth_info(&e, guest_not_user, self->authn_username, self->authn_password);
    network_connection_outbox_push(self, &e);
}

void network_connection_authn_logout(network_connection* self)
{
    event_any e;
    event_create_user_auth_reject(&e, NULL);
    network_connection_outbox_push(self, &e);
}
