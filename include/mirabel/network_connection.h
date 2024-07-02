#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"
#include "mirabel/rrsi.h"

#ifdef __cplusplus
extern "C" {
#endif

static const size_t NETWORK_CONNECTION_MAX_FIELD_SIZE = 128;

typedef struct network_connection_s {
    char adapter_server_address[NETWORK_CONNECTION_MAX_FIELD_SIZE];
    uint16_t adapter_server_port;
    RUNNING_STATE_INDICATOR adapter_state;
    network_adapter neta;

    RUNNING_STATE_INDICATOR connection_state;
    uint8_t connection_cert_thumb[32]; //TODO replace with a sizer for the SHA256 thumbprint
    char* connection_verifail_reason;

    RUNNING_STATE_INDICATOR authinfo_state;
    bool authinfo_allow_login;
    bool authinfo_allow_guest;
    bool authinfo_want_guest_pw;

    char authn_username[NETWORK_CONNECTION_MAX_FIELD_SIZE];
    char authn_password[NETWORK_CONNECTION_MAX_FIELD_SIZE];
    req_res_tracker authn_state;
    char* authn_fail_reason;

    event_queue* outbox;
    event_queue inbox;
} network_connection;

// returns true on failure
bool network_connection_create(network_connection* self);

void network_connection_destroy(network_connection* self);

//TODO returns true on failure
// bool network_connection_event_send_copy(network_connection* self, event_any* e);

#ifdef __cplusplus
}
#endif
