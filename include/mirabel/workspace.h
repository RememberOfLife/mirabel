#pragma once

#include "mirabel/network_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workspace_s {
    network_connection* netc;

    // lobby lobby;
    // session gsession;
    // VECTOR(engine*) engines;
} workspace;

// returns true on failure
bool workspace_create(workspace* self);

void workspace_destroy(workspace* self);

void workspace_process_network_event(workspace* self, event_any* e);

// void workspace_event_send_copy_to_netc(workspace* self, event_any* e);

// void workspace_push_event_to_interface(workspace* self, event_any* e);

//TODO interface interacts with the workspace through methods

#ifdef __cplusplus
}
#endif
