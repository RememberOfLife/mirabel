#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_s {
    // network_adapter* nets;
    // workspace* workspaces; // vector
} client;

// returns true on failure
bool client_create(client* clt);

void client_destroy(client* clt);

// returns true to shutdown
bool client_update(client* clt);

#ifdef __cplusplus
}
#endif
