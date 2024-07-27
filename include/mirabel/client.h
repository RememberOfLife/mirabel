#pragma once

#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/network_connection.h"
#include "mirabel/workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_s {
    VECTOR(network_connection*) net_conns; // owning list of instantiated network adapters
    VECTOR(workspace*) workspaces;
    // own config handle for client, //TODO where do the interface configs go?
} client;

// returns true on failure
bool client_create(client* self);

void client_destroy(client* self);

// returns true to shutdown
bool client_update(client* self);

// interaction methods for interface

// returns NULL on failure, otherwise ptr to the newly created workspace
workspace* client_add_workspace(client* self);

workspace* client_get_workspace_by_id(client* self, uint32_t workspace_id);

network_connection* client_add_connection(client* self);

#ifdef __cplusplus
}
#endif
