#pragma once

#include <stdint.h>

#include "mirabel/network_connection.h"
#include "mirabel/rrsi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workspace_s {
    network_connection* netc;

    uint32_t id;
    RSI server_workspace;

    // lobby lobby;
    // session gsession;
    // VECTOR(engine*) engines;
} workspace;

// returns true on failure
bool workspace_create(workspace* self);

void workspace_destroy(workspace* self);

void workspace_connection_attach(workspace* self, network_connection* netc);

void workspace_connection_detach(workspace* self);

void workspace_netc_send(workspace* self, event_any* e);

// read-only on the event
void workspace_process_network_event(workspace* self, event_any* e);

// void workspace_push_event_to_interface(workspace* self, event_any* e);

//TODO interface interacts with the workspace through methods

void workspace_srvrepr_create(workspace* self);

void workspace_srvrepr_destroy(workspace* self);

#ifdef __cplusplus
}
#endif
