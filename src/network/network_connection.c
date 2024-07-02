#include <stdbool.h>

#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"
#include "mirabel/network_connection.h"

/////
//public

const char* default_adapter_server_address = "run.mirabel.dev";
const uint16_t default_adapter_server_port = 61801;

bool network_connection_create(network_connection* self)
{
    self->adapter_server_address[0] = '\0';
    self->adapter_server_port = default_adapter_server_port;
    self->adapter_state = RSI_IDLE;
    self->adapter.inbox = &self->inbox;

    self->connection_state = RSI_NONE;
    self->connection_verifail_reason = NULL; //TODO unnecessary because RSI_NONE, want to keep it?

    self->authinfo_state = RSI_NONE;

    self->authn_username[0] = '\0'; //TODO unnecessary because RSI_NONE, want to keep it?
    self->authn_password[0] = '\0'; //TODO unnecessary because RSI_NONE, want to keep it?
    self->authn_state.state = RSI_NONE;
    self->authn_fail_reason = NULL; //TODO unnecessary because RSI_NONE, want to keep it?

    self->outbox = &self->adapter.outbox;
    event_queue_create(&self->inbox);
}

void network_connection_destroy(network_connection* self)
{
    //TODO
    event_queue_destroy(&self->inbox);
    //TODO
}

bool network_connection_handle_internal(network_connection* self, event_any* e)
{
    bool consumed = true;
    switch (e->base.type) {
        //TODO our relevant cases..
        default: {
            consumed = false;
        } break;
    }
    return consumed;
}
