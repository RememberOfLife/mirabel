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

#ifdef __cplusplus
}
#endif
