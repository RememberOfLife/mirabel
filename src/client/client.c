#include <stdbool.h>

#include "rosalia/vector.h"

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
                //TODO other event types
                /*
                    for all workspaces
                        if workspace netc is connection
                            if event is relevant for workspace
                                workspace handle event
                */
                default: {
                    mirabel_slogf(LOGS_WARN, "client: connection %zu, received unexpected event, type: %d %s\n", conn_idx, e.base.type, event_type_str(e.base.type));
                } break;
            }
            event_destroy(&e);
        }
    }
    return exit;
}
