#pragma once

#include "mirabel/event_queue.h"
#include "mirabel/network_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workspace_s {
    network_connection* netc;
    event_queue net_inbox; // incoming from network
    event_queue ifc_inbox; // incoming from interface / other controllers

    // lobby lobby;
    // session gsession;
    // VECTOR(engine*) engines;
} workspace;

// returns true on failure
bool workspace_create(workspace* ws);

void workspace_destroy(workspace* ws);

// void workspace_push_event_to_interface(workspace* ws, event_any* e);

#ifdef __cplusplus
}
#endif
