#pragma once

#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/server/lobby_manager.h"
#include "mirabel/server/user_manager.h"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/network_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_connection_s {
    network_adapter* responsible_neta;
    uint32_t neta_local_connection_id;
    uint64_t authn_user_id;
    VECTOR(uint32_t) workspaces; // idcs into server.workspaces
} client_connection;

typedef struct workspace_handle_s {
    uint32_t connection_id; // idx into server.connections
    uint32_t client_local_workspace_id;
    uint32_t lobby_id; //TODO if necessary store heterogenous list of everyone who needs to unregister this handle
} workspace_handle;

typedef struct server_s {
    bool offline;

    VECTOR(network_adapter*) netas;

    event_queue inbox;

    //TODO MAP from neta+neta_local_conn_id -> connection_id (idx into server.connections)
    VECTOR(client_connection) connections; // connections start at 1;
    VECTOR(workspace_handle) workspaces; // server global workspaces

    server_user_manager user_mgr;
    server_lobby_manager lobby_mgr;

    //TODO
    // own config handle for server
    // db connection
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

void server_connection_network_send(server* self, uint32_t connection_id, event_any* e);
void server_connection_network_send_delayed(server* self, uint32_t connection_id, event_any* e, uint32_t delay_ms);

uint32_t server_workspace_handle_add(server* add, uint32_t connection_id, uint32_t client_local_workspace_id);

void server_workspace_handle_remove(server* add, uint32_t workspace_id);

// returns 0 if it can not be found
uint32_t server_workspace_handle_get(server* add, uint32_t connection_id, uint32_t client_local_workspace_id);

void server_workspace_handle_network_send(server* self, uint32_t workspace_id, event_any* e);
void server_workspace_handle_network_send_delayed(server* self, uint32_t workspace_id, event_any* e, uint32_t delay_ms);

#ifdef __cplusplus
}
#endif
