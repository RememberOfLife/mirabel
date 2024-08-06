#include <stdbool.h>
#include <stdint.h>

#include "mirabel/event.h"

#include "mirabel/workspace.h"

uint32_t next_workspace_id = 1;

bool workspace_create(workspace* self)
{
    self->netc = NULL;
    self->id = next_workspace_id++;
    self->server_workspace = RSI_NONE; // marks that this workspace has never had or attempted a server workspace, used to automatically request workspace creation on successful connection
    return false;
}

void workspace_destroy(workspace* self)
{
    //TODO if wanted, notify client that network connection has one less user so maybe it can be shutdown
}

void workspace_connection_attach(workspace* self, network_connection* netc)
{
    self->netc = netc;
}

void workspace_connection_detach(workspace* self)
{
    //TODO properly unregister on server etc..
    self->netc = NULL;
    self->id = next_workspace_id++;
    self->server_workspace = RSI_NONE;
}

void workspace_netc_send(workspace* self, event_any* e)
{
    bool consumed = false;
    switch (e->base.type) {
        case EVENT_TYPE_WORKSPACE_CREATE: {
            self->server_workspace = RSI_WAITING;
        } break;
        case EVENT_TYPE_WORKSPACE_DESTROY: {
            self->server_workspace = RSI_WAITING;
        } break;
        //TODO our relevant cases..
        default: {
            // pass
        } break;
    }
    if (consumed) {
        event_destroy(e);
    } else {
        network_connection_outbox_push(self->netc, e);
    }
}

void workspace_process_network_event(workspace* self, event_any* e)
{
    switch (e->base.type) {
        case EVENT_TYPE_WORKSPACE_CREATE: {
            self->server_workspace = RSI_DONE;
        } break;
        case EVENT_TYPE_WORKSPACE_DESTROY: {
            self->server_workspace = RSI_IDLE;
        } break;
        //TODO our relevant cases..
        default: {
            // no default foward handling, so its a warning
            mirabel_slogf(LOGS_WARN, "workspace %u: received unexpected event type %u %s", self->id, e->base.type, event_type_str(e->base.type));
        } break;
    }
}

void workspace_srvrepr_create(workspace* self)
{
    event_any e;
    event_create_type_workspace(&e, EVENT_TYPE_WORKSPACE_CREATE, self->id);
    workspace_netc_send(self, &e);
}

void workspace_srvrepr_destroy(workspace* self)
{
    event_any e;
    event_create_type_workspace(&e, EVENT_TYPE_WORKSPACE_DESTROY, self->id);
    workspace_netc_send(self, &e);
}
