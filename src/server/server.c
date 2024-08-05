#include "network/adapters/offline_server.h"

#include "rosalia/vector.h"

#include "mirabel/alloc.h"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "mirabel/server/lobby_manager.h"
#include "mirabel/server/user_manager.h"

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
                mirabel_slogf(LOGS_WARN, "server: received unexpected event, type: %d %s", e.base.type, event_type_str(e.base.type));
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
        //TODO can we somehow directly translate the connection id, here! ?
        bool consumed = true;
        switch (e.base.type) {
            case EVENT_TYPE_NULL: {
                remaining_budget = 0;
            } break;
            case EVENT_TYPE_EXIT: {
                remaining_budget = 0;
                break;
            } break;
            case EVENT_TYPE_LOG: {
                mirabel_slogf(e.log.status, "server adapter-handling: queue log: %s", e.log.str);
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_OPEN: {
                uint32_t origin_conn_id = server_client_connection_get(self, neta, e.base.connection_id);
                if (origin_conn_id != EVENT_CONNECTION_NONE) {
                    mirabel_slogf(LOGS_WARN, "server adapter-handling: received connection open for already existing connection %u", origin_conn_id);
                    break;
                }
                server_client_connection_add(self, neta, e.base.connection_id);
                consumed = false;
            } break;
            case EVENT_TYPE_NETWORK_CONNECTION_CLOSE: {
                uint32_t origin_conn_id = server_client_connection_get(self, neta, e.base.connection_id);
                //TODO make lobbies remove all workspace_handles pertaining to this connection, by simply removing the workspace handle?, or does removing the connection also remove all the handles automatically?
                server_client_connection_remove(self, origin_conn_id);
                consumed = false;
            } break;
            //TODO here or elsewhere?
            case EVENT_TYPE_WORKSPACE_CREATE: {
                //TODO
            } break;
            case EVENT_TYPE_WORKSPACE_DESTROY: {
                //TODO
            } break;
            //TODO more (consumed) event types?
            default: {
                consumed = false;
            } break;
        }
        if (consumed) {
            event_destroy(&e);
        } else {
            uint32_t origin_conn_id = server_client_connection_get(self, neta, e.base.connection_id);
            if (origin_conn_id == EVENT_CONNECTION_NONE) {
                mirabel_slogf(LOGS_WARN, "server adapter-handling: event from unknown connection %u received, type %u %s", origin_conn_id, e.base.type, event_type_str(e.base.type));
            } else {
                e.base.connection_id = origin_conn_id;
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
        client_connection new_client_conn = (client_connection){
            .responsible_neta = neta,
            .neta_local_connection_id = neta_local_connection_id,
        };
        VEC_PUSH(&self->connections, new_client_conn);
        new_conn_id = VEC_LEN(&self->connections) - 1;
    } else {
        // reuse existing slot
        new_conn_id = self->connections[0].neta_local_connection_id;
        self->connections[0].neta_local_connection_id = self->connections[new_conn_id].neta_local_connection_id;
        self->connections[new_conn_id] = (client_connection){
            .responsible_neta = neta,
            .neta_local_connection_id = neta_local_connection_id,
        };
    }
    return new_conn_id;
}

void server_client_connection_remove(server* self, uint32_t connection_id)
{
    self->connections[connection_id] = (client_connection){
        .responsible_neta = NULL,
        .neta_local_connection_id = self->connections[0].neta_local_connection_id,
    };
    self->connections[0].neta_local_connection_id = connection_id;
}

uint32_t server_client_connection_get(server* self, network_adapter* neta, uint32_t neta_local_connection_id)
{
    //TODO use map to make this faster
    for (size_t conn_id = 1; conn_id < VEC_LEN(&self->connections); conn_id++) {
        if (self->connections[conn_id].responsible_neta == neta && self->connections[conn_id].neta_local_connection_id == neta_local_connection_id) {
            return conn_id;
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
    client_connection* cc = &self->connections[connection_id];
    if (cc->responsible_neta == NULL) {
        mirabel_slogf(LOGS_ERR, "server client-connection: connection %u, responsible network adapter missing\nsend event dropped, type %u %s", connection_id, e->base.type, event_type_str(e->base.type));
        return;
    }
    e->base.connection_id = cc->neta_local_connection_id;
    e->base.workspace_id = EVENT_WORKSPACE_NONE;
    event_queue_push_delayed(&cc->responsible_neta->outbox, e, delay_ms);
}

uint32_t server_workspace_handle_add(server* add, uint32_t connection_id, uint32_t client_local_workspace_id)
{
    //TODO
}

void server_workspace_handle_remove(server* add, uint32_t workspace_id)
{
    //TODO
}

// returns 0 if it can not be found
uint32_t server_workspace_handle_get(server* add, uint32_t connection_id, uint32_t client_local_workspace_id)
{
    //TODO
}

void server_workspace_handle_network_send(server* self, uint32_t workspace_id, event_any* e)
{
    server_workspace_handle_network_send_delayed(self, workspace_id, e, 0);
}

void server_workspace_handle_network_send_delayed(server* self, uint32_t workspace_id, event_any* e, uint32_t delay_ms)
{
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
