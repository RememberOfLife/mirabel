#pragma once

#include <stdbool.h>
#include <stdint.h>

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
    //TODO bool deleted; // adapter is destructing itself asynchronously, will issue an event when it is ready to be destructed

    char adapter_server_address[128];
    uint16_t adapter_server_port;
    //TODO adapter type selector
    RUNNING_STATE_INDICATOR adapter_state;
    network_adapter adapter;

    RUNNING_STATE_INDICATOR connection_state;
    uint8_t connection_cert_thumb[32]; //TODO replace with a sizer for the SHA256 thumbprint
    char* connection_verifail_reason;

    RUNNING_STATE_INDICATOR authinfo_state;
    bool authinfo_allow_login;
    bool authinfo_allow_guest;
    bool authinfo_want_guest_pw;

    char authn_username[64];
    char authn_password[128];
    req_res_tracker authn_state;
    char* authn_fail_reason;

    event_queue* outbox;
    event_queue inbox;
} network_connection;

// returns true on failure
bool network_connection_create(network_connection* self);

void network_connection_destroy(network_connection* self);

// returns true if the event was consumed internally
bool network_connection_handle_internal(network_connection* self, event_any* e);

//TODO returns true on failure
// bool network_connection_event_send_copy(network_connection* self, event_any* e);

#ifdef __cplusplus
}
#endif
