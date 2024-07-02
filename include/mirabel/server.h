#pragma once

#include <stdbool.h>

#include "mirabel/event_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct server_s {
    bool offline;
    event_queue inbox; //TODO if the server gets multithreaded then we need some more complicated queue stealing anyway (i.e. every network adapter just enqueues in its recv_box and the threads work steal from all the adapters round robin so even if one adapter has more, we still process others faily)

    //TODO
    // own config handle for server
    // db connection
    // network_connection* netc; // where events come in
    // connection* connections; // active connections to clients
    // user_info* users;
    // lobby* lobbies;
    // session* sessions;
} server;

// returns true on failure
bool server_create(server* self, bool offline);

void server_destroy(server* self);

// returns true to shutdown
bool server_update(server* self);

#ifdef __cplusplus
}
#endif
