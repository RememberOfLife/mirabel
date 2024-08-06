#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/alloc.h"
#include "mirabel/workspace.h"

#include "mirabel/client.h"

bool client_create(client* self)
{
    VEC_CREATE(&self->net_conns, 2);
    VEC_CREATE(&self->workspaces, 1);
    return false;
}

void client_destroy(client* self)
{
    for (size_t i = 0; i < VEC_LEN(&self->net_conns); i++) {
        //TODO destroy network connection
    }
    VEC_DESTROY(&self->net_conns);
    for (size_t i = 0; i < VEC_LEN(&self->workspaces); i++) {
        workspace_destroy(self->workspaces[i]);
    }
    VEC_DESTROY(&self->workspaces);
}

bool client_update(client* self)
{
    bool exit = false;
    //TODO
    for (size_t conn_idx = 0; conn_idx < VEC_LEN(&self->net_conns); conn_idx++) {
        network_connection* conn = self->net_conns[conn_idx];
        int32_t remaining_budget = 1024; // limit maximum event processing if queue is too big
        while (remaining_budget > 0) {
            remaining_budget--;
            event_any e;
            network_connection_inbox_pop(conn, &e);
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
                    mirabel_slogf(e.log.status, "client: queue log: %s", e.log.str);
                } break;
                //TODO other handled event types
                default: {
                    if (e.base.workspace_id == EVENT_WORKSPACE_NONE) {
                        mirabel_slogf(LOGS_WARN, "client: connection %zu, received unexpected event, type: %u %s with no workspace", conn_idx, e.base.type, event_type_str(e.base.type));
                    } else {
                        workspace* target_ws = client_get_workspace_by_id(self, e.base.workspace_id);
                        if (target_ws == NULL) {
                            mirabel_slogf(LOGS_WARN, "client: connection %zu, event type %u %s could not be delivered to workspace %u, could not find workspace", conn_idx, e.base.type, event_type_str(e.base.type), e.base.workspace_id);
                        } else {
                            workspace_process_network_event(target_ws, &e);
                        }
                    }
                } break;
            }
            event_destroy(&e);
        }
    }
    return exit;
}

workspace* client_add_workspace(client* self)
{
    workspace* new_workspace = mirabel_malloc(sizeof(workspace));
    if (workspace_create(new_workspace)) {
        mirabel_free(new_workspace);
        return NULL;
    }
    VEC_PUSH(&self->workspaces, new_workspace);
    return new_workspace;
}

workspace* client_get_workspace_by_id(client* self, uint32_t workspace_id)
{
    for (size_t widx = 0; widx < VEC_LEN(&self->workspaces); widx++) {
        if (self->workspaces[widx]->id == workspace_id) {
            return self->workspaces[widx];
        }
    }
    return NULL;
}

network_connection* client_add_connection(client* self)
{
    network_connection* new_net_conn = mirabel_malloc(sizeof(network_connection));
    if (network_connection_create(new_net_conn)) {
        mirabel_free(new_net_conn);
        return NULL;
    }
    VEC_PUSH(&self->net_conns, new_net_conn);
    return new_net_conn;
}
