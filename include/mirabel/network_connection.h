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

//TODO any way to NOT make these macros?
#define ADAPTER_SERVER_ADDRESS_SIZE (128)
#define CONNECTION_AUTHN_USERNAME_SIZE (64)
#define CONNECTION_AUTHN_PASSWORD_SIZE (128)

extern const char* default_adapter_server_address;
extern const uint16_t default_adapter_server_port;

typedef struct network_connection_s {
    bool deleted; // adapter is destructing itself asynchronously, will issue an event when it is ready to be destructed

    char adapter_server_address[ADAPTER_SERVER_ADDRESS_SIZE];
    uint16_t adapter_server_port;
    RSI adapter_state;
    network_adapter adapter;
    char* adapter_error;

    RSI connection_state;
    blob connection_cert_thumb;
    char* connection_verifail_reason; // owning

    RSI authinfo_state;
    bool authinfo_allow_login;
    bool authinfo_allow_guest;
    bool authinfo_want_guest_pw;

    char authn_username[CONNECTION_AUTHN_USERNAME_SIZE];
    char authn_password[CONNECTION_AUTHN_PASSWORD_SIZE];
    RSI authn_state;
    char* authn_fail_reason; // owning

    //TODO need one more bool / tracker for having finished?
    // only access these through inbox_pop and outbox_push
    event_queue* outbox;
    event_queue inbox;

    // VECTOR(size_t) connected_workspace_idcs; //TODO would want this to directly target the relevant workspaces, and not have to ptr compare the workspaces network_connection ptr to the one where the event arrived from, when matching, also need it to inform workspaces when the connection closes and opens
} network_connection;

// returns true on failure
bool network_connection_create(network_connection* self);

void network_connection_destroy(network_connection* self);

// like event_queue_push
void network_connection_outbox_push(network_connection* self, event_any* e);

// non blocking, like event_queue_pop
void network_connection_inbox_pop(network_connection* self, event_any* e);

void network_connection_adapter_open(network_connection* self);

void network_connection_adapter_close(network_connection* self);

void network_connection_ping(network_connection* self);

void network_connection_veriaccept(network_connection* self);

void network_connection_authn_login(network_connection* self, bool guest_not_user);

void network_connection_authn_logout(network_connection* self);

#ifdef __cplusplus
}
#endif
