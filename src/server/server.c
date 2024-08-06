#include <assert.h>

#include "rosalia/vector.h"

#include "mirabel/server/lobby_manager.h"
#include "mirabel/server/user_manager.h"
#include "mirabel/alloc.h"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "network/adapters/offline_server.h"

#include "mirabel/server.h"

bool server_create(server* self, bool offline)
{
    self->offline = offline;

    VEC_CREATE(&self->netas, offline ? 1 : 4);
    network_adapter* offline_neta_server = mirabel_malloc(sizeof(network_adapter));
    offline_neta_server->methods = &offline_server_network_adapter_methods;
    offline_neta_server->inbox = mirabel_malloc(sizeof(event_queue));
    event_queue_create(offline_neta_server->inbox);
    VEC_PUSH(&self->netas, offline_neta_server);
    network_adapter_create(offline_neta_server);

    event_queue_create(&self->inbox);

    VEC_CREATE(&self->connections, 16);
    client_connection client_connection_free_head = (client_connection){
        .responsible_neta = NULL,
        .neta_local_connection_id = EVENT_CONNECTION_NONE,
        .authn_user_id = USER_ID_NONE,
    };
    VEC_CREATE(&client_connection_free_head.workspaces, 0);
    VEC_PUSH(&self->connections, client_connection_free_head);

    VEC_CREATE(&self->workspaces, 16);
    workspace_handle workspace_handle_free_head = (workspace_handle){
        .connection_id = EVENT_CONNECTION_NONE,
        .client_local_workspace_id = EVENT_WORKSPACE_NONE,
        .lobby_id = LOBBY_ID_NONE,
    };
    VEC_PUSH(&self->workspaces, workspace_handle_free_head);

    server_user_manager_create(&self->user_mgr);
    server_lobby_manager_create(&self->lobby_mgr);

    return false;
}

void server_destroy(server* self)
{
    //TODO order fine, and/or do we want to do more things?

    server_lobby_manager_destroy(&self->lobby_mgr);

    server_user_manager_destroy(&self->user_mgr);

    VEC_DESTROY(&self->workspaces);

    for (size_t conn_idx = 0; conn_idx < VEC_LEN(&self->connections); conn_idx++) {
        VEC_DESTROY(&self->connections[conn_idx].workspaces);
    }
    VEC_DESTROY(&self->connections);

    for (size_t neta_idx = 0; neta_idx < VEC_LEN(&self->netas); neta_idx++) {
        network_adapter_destroy(self->netas[neta_idx]);
        event_queue_destroy(self->netas[neta_idx]->inbox);
        mirabel_free(self->netas[neta_idx]->inbox);
        mirabel_free(self->netas[neta_idx]);
    }
    VEC_DESTROY(&self->netas);

    event_queue_destroy(&self->inbox);
}

bool server_update(server* self)
{
    for (size_t neta_idx = 0; neta_idx < VEC_LEN(&self->netas); neta_idx++) {
        server_handle_adapter_incoming(self, self->netas[neta_idx]);
    }

    bool exit = false;
    int32_t remaining_budget = 1024; // limit maximum event processing if queue is too big
    while (remaining_budget > 0) {
        remaining_budget--;
        event_any e;
        event_queue_pop(&self->inbox, &e, 0); //TODO for a true ONLY server, we will end spinning a lot if we do this
        switch (e.base.type) {
            case EVENT_TYPE_NULL: {
                remaining_budget = 0;
            } break;
            case EVENT_TYPE_EXIT: {
                remaining_budget = 0;
                exit = true;
                break;
            } break;
            case EVENT_TYPE_LOG: {
                mirabel_slogf(e.log.status, "server: queue log: %s", e.log.str);
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_OPEN: {
                mirabel_slogf(LOGS_OK, "server: connection %u open", e.base.connection_id);
                server_user_manager_handle_event(&self->user_mgr, &e);
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_CLOSE: {
                mirabel_slogf(LOGS_OK, "server: connection %u close", e.base.connection_id);
                //TODO cleanup on server
            } break;
            case EVENT_TYPE_USER_AUTH_INFO: {
                server_user_manager_handle_event(&self->user_mgr, &e);
            } break;
            case EVENT_TYPE_USER_AUTH_REJECT: {
                server_user_manager_handle_event(&self->user_mgr, &e);
            } break;
            //TODO other event types
            default: {
                mirabel_slogf(LOGS_WARN, "server: received unexpected event, type: %u %s", e.base.type, event_type_str(e.base.type));
            } break;
        }
        event_destroy(&e);
    }
    return exit;
}

void server_handle_adapter_incoming(server* self, network_adapter* neta)
{
    int32_t remaining_budget = 1024;
    while (remaining_budget > 0) {
        remaining_budget--;
        event_any e;
        event_queue_pop(neta->inbox, &e, 0); //TODO for a true ONLY server, we will end spinning a lot if we do this
        uint32_t server_global_connection_id = server_client_connection_get(self, neta, e.base.connection_id);
        uint32_t server_global_workspace_id = server_workspace_handle_get(self, server_global_connection_id, e.base.workspace_id);
        //TODO can we somehow directly translate the connection id, here! ?
        bool consumed = false;
        switch (e.base.type) {
            case EVENT_TYPE_NULL: {
                remaining_budget = 0;
                consumed = true;
                break;
            } break;
            case EVENT_TYPE_EXIT: {
                remaining_budget = 0;
                consumed = true;
                break;
            } break;
            case EVENT_TYPE_LOG: {
                mirabel_slogf(e.log.status, "server adapter-handling: queue log: %s", e.log.str);
                consumed = true;
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_OPEN: {
                if (server_global_connection_id != EVENT_CONNECTION_NONE) {
                    mirabel_slogf(LOGS_WARN, "server adapter-handling: received connection open for already existing connection %u", server_global_connection_id);
                    consumed = true;
                    break;
                }
                server_global_connection_id = server_client_connection_add(self, neta, e.base.connection_id);
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_CLOSE: {
                // removing the connection also automatically removes all handles and informs all relevant participants
                server_client_connection_remove(self, server_global_connection_id);
            } break;
            case EVENT_TYPE_WORKSPACE_CREATE: {
                consumed = true;
                //TODO error if conn none
                if (server_global_workspace_id != EVENT_WORKSPACE_NONE) {
                    mirabel_slogf(LOGS_WARN, "server adapter-handling: received workspace create for already created workspace %u", server_global_workspace_id);
                    break;
                }
                server_global_workspace_id = server_workspace_handle_add(self, server_global_connection_id, e.base.workspace_id);
                event_any re;
                event_create_type(&re, EVENT_TYPE_WORKSPACE_CREATE);
                server_workspace_handle_network_send(self, server_global_workspace_id, &re);
            } break;
            case EVENT_TYPE_WORKSPACE_DESTROY: {
                consumed = true;
                //TODO error if conn none
                //TODO error if wsh none
                event_any re;
                event_create_type(&re, EVENT_TYPE_WORKSPACE_DESTROY);
                server_workspace_handle_network_send(self, server_global_workspace_id, &re);
                //removing the handle also automatically informs all relevant participants
                server_workspace_handle_remove(self, server_global_workspace_id);
            } break;
            //TODO more (consumed) event types?
            default: {
                // pass
            } break;
        }
        if (consumed) {
            event_destroy(&e);
        } else {
            if (server_global_connection_id == EVENT_CONNECTION_NONE) {
                mirabel_slogf(LOGS_WARN, "server adapter-handling: event from unknown connection %u can not be forwarded, type %u %s", server_global_connection_id, e.base.type, event_type_str(e.base.type));
            } else {
                e.base.connection_id = server_global_connection_id;
                e.base.workspace_id = server_global_workspace_id;
                event_queue_push(&self->inbox, &e);
            }
        }
    }
}

uint32_t server_client_connection_add(server* self, network_adapter* neta, uint32_t neta_local_connection_id)
{
    uint32_t new_conn_id;
    if (self->connections[0].neta_local_connection_id == EVENT_CONNECTION_NONE) {
        // push new connection
        new_conn_id = VEC_LEN(&self->connections);
        VEC_PUSH_N(&self->connections, 1);
    } else {
        // reuse existing slot
        new_conn_id = self->connections[0].neta_local_connection_id;
        self->connections[0].neta_local_connection_id = self->connections[new_conn_id].neta_local_connection_id;
    }
    assert(new_conn_id != EVENT_CONNECTION_NONE);
    self->connections[new_conn_id] = (client_connection){
        .responsible_neta = neta,
        .neta_local_connection_id = neta_local_connection_id,
    };
    return new_conn_id;
}

void server_client_connection_remove(server* self, uint32_t connection_id)
{
    // remove all handles this connection has
    for (size_t wsh_idx = 0; wsh_idx < VEC_LEN(&self->connections[connection_id].workspaces); wsh_idx++) {
        server_workspace_handle_remove(self, self->connections[connection_id].workspaces[wsh_idx]);
    }
    // actually remove
    VEC_DESTROY(&self->connections[connection_id].workspaces);
    self->connections[connection_id] = (client_connection){
        .responsible_neta = NULL,
        .neta_local_connection_id = self->connections[0].neta_local_connection_id,
        .authn_user_id = USER_ID_NONE,
        .workspaces = NULL,
    };
    self->connections[0].neta_local_connection_id = connection_id;
}

uint32_t server_client_connection_get(server* self, network_adapter* neta, uint32_t neta_local_connection_id)
{
    //TODO use map to make this faster
    for (size_t conn_idx = 1; conn_idx < VEC_LEN(&self->connections); conn_idx++) {
        if (self->connections[conn_idx].responsible_neta == neta && self->connections[conn_idx].neta_local_connection_id == neta_local_connection_id) {
            return conn_idx;
        }
    }
    return EVENT_CONNECTION_NONE;
}

void server_connection_network_send(server* self, uint32_t connection_id, event_any* e)
{
    server_connection_network_send_delayed(self, connection_id, e, 0);
}

void server_connection_network_send_delayed(server* self, uint32_t connection_id, event_any* e, uint32_t delay_ms)
{
    if (connection_id == EVENT_CONNECTION_NONE) {
        mirabel_slogf(LOGS_ERR, "server client-connection: can not send for connection NONE\nsend event dropped, type %u %s", e->base.type, event_type_str(e->base.type));
        return;
    }
    client_connection* cc = &self->connections[connection_id];
    if (cc->responsible_neta == NULL) {
        mirabel_slogf(LOGS_ERR, "server client-connection: connection %u, responsible network adapter missing\nsend event dropped, type %u %s", connection_id, e->base.type, event_type_str(e->base.type));
        return;
    }
    e->base.connection_id = cc->neta_local_connection_id;
    e->base.workspace_id = EVENT_WORKSPACE_NONE;
    event_queue_push_delayed(&cc->responsible_neta->outbox, e, delay_ms);
}

uint32_t server_workspace_handle_add(server* self, uint32_t connection_id, uint32_t client_local_workspace_id)
{
    uint32_t new_wsh_id;
    if (self->workspaces[0].client_local_workspace_id == EVENT_WORKSPACE_NONE) {
        // push new connection
        new_wsh_id = VEC_LEN(&self->workspaces);
        VEC_PUSH_N(&self->workspaces, 1);
    } else {
        // reuse existing slot
        new_wsh_id = self->workspaces[0].client_local_workspace_id;
        self->workspaces[0].client_local_workspace_id = self->workspaces[new_wsh_id].client_local_workspace_id;
    }
    assert(new_wsh_id != EVENT_WORKSPACE_NONE);
    self->workspaces[new_wsh_id] = (workspace_handle){
        .connection_id = connection_id,
        .client_local_workspace_id = client_local_workspace_id,
        .lobby_id = LOBBY_ID_NONE,
    };
    VEC_PUSH(&self->connections[connection_id].workspaces, new_wsh_id);
    return new_wsh_id;
}

void server_workspace_handle_remove(server* self, uint32_t workspace_id)
{
    //TODO notify lobby that the handle has been dropped
    // actually remove
    self->workspaces[workspace_id] = (workspace_handle){
        .connection_id = EVENT_CONNECTION_NONE,
        .client_local_workspace_id = self->workspaces[0].client_local_workspace_id,
        .lobby_id = LOBBY_ID_NONE,
    };
    self->workspaces[0].client_local_workspace_id = workspace_id;
}

uint32_t server_workspace_handle_get(server* self, uint32_t connection_id, uint32_t client_local_workspace_id)
{
    //TODO use map to make this faster
    for (size_t wsh_idx = 1; wsh_idx < VEC_LEN(&self->workspaces); wsh_idx++) {
        if (self->workspaces[wsh_idx].connection_id == connection_id && self->workspaces[wsh_idx].client_local_workspace_id == client_local_workspace_id) {
            return wsh_idx;
        }
    }
    return EVENT_WORKSPACE_NONE;
}

void server_workspace_handle_network_send(server* self, uint32_t workspace_id, event_any* e)
{
    server_workspace_handle_network_send_delayed(self, workspace_id, e, 0);
}

void server_workspace_handle_network_send_delayed(server* self, uint32_t workspace_id, event_any* e, uint32_t delay_ms)
{
    if (workspace_id == EVENT_WORKSPACE_NONE) {
        mirabel_slogf(LOGS_ERR, "server workspace-handle: can not send for workspace NONE\nsend event dropped, type %u %s", e->base.type, event_type_str(e->base.type));
        return;
    }
    workspace_handle* wh = &self->workspaces[workspace_id];
    client_connection* cc = &self->connections[wh->connection_id];
    if (cc->responsible_neta == NULL) {
        mirabel_slogf(LOGS_ERR, "server workspace-handle: workspace %u, connection %u, responsible network adapter missing\nsend event dropped, type %u %s", workspace_id, wh->connection_id, e->base.type, event_type_str(e->base.type));
        return;
    }
    e->base.connection_id = cc->neta_local_connection_id;
    e->base.workspace_id = wh->client_local_workspace_id;
    event_queue_push_delayed(&cc->responsible_neta->outbox, e, delay_ms);
}
