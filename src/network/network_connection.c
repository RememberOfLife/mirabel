#include <stdbool.h>

#include "rosalia/serialization.h"

#include "mirabel/alloc.h"
#include "mirabel/event_queue.h"
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
    self->adapter_error = NULL; //TODO unnecessary because RSI_IDLE, want to keep it?

    self->connection_state = RSI_NONE;
    self->connection_cert_thumb = BLOB_NULL;
    self->connection_verifail_reason = NULL; //TODO unnecessary because RSI_NONE, want to keep it?

    self->authinfo_state = RSI_NONE;

    self->authn_username[0] = '\0'; //TODO unnecessary because RSI_NONE, want to keep it?
    self->authn_password[0] = '\0'; //TODO unnecessary because RSI_NONE, want to keep it?
    self->authn_state.state = RSI_NONE;
    self->authn_fail_reason = NULL; //TODO unnecessary because RSI_NONE, want to keep it?

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
    bool consumed = true;
    switch (e->base.type) {
        //TODO our relevant cases..
        case EVENT_TYPE_NETWORK_ADAPTER_OPEN: {
            self->adapter_state = RSI_WAITING;
            if (self->adapter_error != NULL) {
                mirabel_free(self->adapter_error);
                self->adapter_error = NULL;
            }
            network_adapter_create(&self->adapter);
            consumed = false; // forward to the adapter
        } break;
        case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {

        } break;
        default: {
            consumed = false;
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
            //TODO our relevant cases..
            case EVENT_TYPE_NETWORK_ADAPTER_OPEN: {
                self->adapter_state = RSI_DONE;
                if (self->connection_verifail_reason != NULL) {
                    mirabel_free(self->connection_verifail_reason);
                    self->connection_verifail_reason = NULL;
                }
                self->connection_state = RSI_WAITING;
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                mirabel_slogf(LOGS_OK, "connection rx close: %s %hu", e->neta_open.server_addr, e->neta_open.server_port); //REMOVE
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT: {
                blob_copy(&self->connection_cert_thumb, &e->neta_veri.thumb); //TODO use blob_move when available
                self->connection_state = RSI_DONE;
                self->authinfo_state = RSI_WAITING;
            } break;
            case EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT: {
                mirabel_slogf(LOGS_OK, "connection rx verireject: %s %hu", e->neta_open.server_addr, e->neta_open.server_port); //REMOVE
            } break;
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
