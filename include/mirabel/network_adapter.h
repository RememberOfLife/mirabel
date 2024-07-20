#pragma once

#include <stdbool.h>

#include "rosalia/semver.h"

#include "mirabel/event_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct network_adapter_s network_adapter;

typedef const char* network_adapter_get_last_error_t(network_adapter* self);

// the network adapter is responsible for spawning any required threads itself
// returns true on fail
// if this fails, call destroy
typedef bool network_adapter_create_t(network_adapter* self);

// we *can* but do not *need* to event destruct the adapter beforehand
// we can either just use destroy, or even destruct and the nuse destroy, both works
typedef void network_adapter_destroy_t(network_adapter* self);

typedef struct network_adapter_methods_s {

    const char* name;
    const semver version;

    network_adapter_get_last_error_t* get_last_error;
    network_adapter_create_t* create;
    network_adapter_destroy_t* destroy;

} network_adapter_methods;

struct network_adapter_s {
    const network_adapter_methods* methods;
    void* data; // owned by the methods
    event_queue outbox; // the adapter reads these and potentially sends them through the connection
    event_queue* inbox; // points to the inbox where the adapter will place events it generated or received from the connection
};

network_adapter_get_last_error_t network_adapter_get_last_error;
network_adapter_create_t network_adapter_create;
network_adapter_destroy_t network_adapter_destroy;

#ifdef __cplusplus
}
#endif
