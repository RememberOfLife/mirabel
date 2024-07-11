#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rosalia/vector.h"

#include "mirabel/event.h"
#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"
#include "mirabel/rrsi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char* default_adapter_server_address;
extern const uint16_t default_adapter_server_port;

typedef struct network_connection_s {
    bool deleted; // adapter is destructing itself asynchronously, will issue an event when it is ready to be destructed

    char adapter_server_address[128];
    uint16_t adapter_server_port;
    char* adapter_methods_name; // owning
    RUNNING_STATE_INDICATOR adapter_state;
    network_adapter adapter;
    char* adapter_error;

    RUNNING_STATE_INDICATOR connection_state;
    uint8_t connection_cert_thumb[32]; //TODO replace with a sizer for the SHA256 thumbprint
    char* connection_verifail_reason; // owning

    RUNNING_STATE_INDICATOR authinfo_state;
    bool authinfo_allow_login;
    bool authinfo_allow_guest;
    bool authinfo_want_guest_pw;

    char authn_username[64];
    char authn_password[128];
    req_res_tracker authn_state;
    char* authn_fail_reason; // owning

    //TODO need one more bool / tracker for having finished?
    // only access these through inbox_pop and outbox_push
    event_queue* outbox;
    event_queue inbox;

    VECTOR(size_t) connected_workspace_idcs;
} network_connection;

// returns true on failure
bool network_connection_create(network_connection* self);

void network_connection_destroy(network_connection* self);

// like event_queue_push
void network_connection_outbox_push(network_connection* self, event_any* e);

// non blocking, like event_queue_pop
void network_connection_inbox_pop(network_connection* self, event_any* e);

#ifdef __cplusplus
}
#endif
