#pragma once

#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/network_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_connection_s {
    network_adapter* responsible_neta;
    uint32_t neta_local_connection_id;
    //TODO authenticated user, if any
    //TODO generation index
} client_connection;

typedef struct workspace_observer_s {
    uint32_t connection_id; // idx into server.connections
    //TODO generation index for connection
    uint32_t workspace_id;
} workspace_observer;

typedef struct server_s {
    bool offline;

    VECTOR(network_adapter*) netas; //TODO better datastructure for sending events outwards, this does not capture any association between a connection and an adapter, or even just a quick way to find the appropriate adapter if we knew it..

    event_queue inbox; //TODO if the server gets multithreaded then we need some more complicated queue stealing anyway (i.e. every network adapter just enqueues in its recv_box and the threads work steal from all the adapters round robin so even if one adapter has more, we still process others fairly)

    //TODO MAP from neta+neta_local_conn_id -> connection_id (idx into server.connections)
    VECTOR(client_connection) connections; // connections start at 1;

    //TODO
    // own config handle for server
    // db connection
    // user_info* users;
    // lobby* lobbies;
    // session* sessions;
} server;

// returns true on failure
bool server_create(server* self, bool offline);

void server_destroy(server* self);

// returns true to shutdown
bool server_update(server* self);

// polls from adapter and places into inbox, translates connection ids between
void server_handle_adapter_incoming(server* self, network_adapter* neta);

// returns the connection_id of the newly created connection
uint32_t server_client_connection_add(server* self, network_adapter* neta, uint32_t neta_local_connection_id);

void server_client_connection_remove(server* self, uint32_t connection_id);

// returns EVENT_CONNECTION_NONE if it can not be found
uint32_t server_client_connection_get(server* self, network_adapter* neta, uint32_t neta_local_connection_id);

//TODO move away to proper place
//TODO makes assumptions about existance of connection, also AB problem! need generation idx for connections?
void server_workspace_observer_network_send(server* self, workspace_observer* ws_ob, event_any* e);

#ifdef __cplusplus
}
#endif
