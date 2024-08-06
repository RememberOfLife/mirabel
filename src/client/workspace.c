#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "mirabel/server/lobby.h"
#include "mirabel/alloc.h"
#include "mirabel/event.h"

#include "mirabel/workspace.h"

uint32_t next_workspace_id = 1;

bool workspace_create(workspace* self)
{
    self->netc = NULL;
    self->id = next_workspace_id++;
    self->server_workspace = RSI_NONE; // marks that this workspace has never had or attempted a server workspace, used to automatically request workspace creation on successful connection

    self->lobby_name[0] = '\0';
    self->lobby_password[0] = '\0';
    self->lobby_state = RSI_IDLE;
    self->lobby_error = NULL;
    self->lobby_id = EVENT_LOBBY_NONE;

    return false;
}

void workspace_destroy(workspace* self)
{
    //TODO if wanted, notify client that network connection has one less user so maybe it can be shutdown

    if (self->lobby_error != NULL) {
        mirabel_free(self->lobby_error);
    }
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
        e->base.workspace_id = self->id;
        network_connection_outbox_push(self->netc, e);
    }
}

void workspace_process_network_event(workspace* self, event_any* e)
{
    assert(e->base.workspace_id == self->id);
    switch (e->base.type) {
        case EVENT_TYPE_WORKSPACE_CREATE: {
            self->server_workspace = RSI_DONE;
        } break;
        case EVENT_TYPE_WORKSPACE_DESTROY: {
            self->server_workspace = RSI_IDLE;
        } break;
        case EVENT_TYPE_LOBBY_JOIN: {
            //TODO
            mirabel_slogf(LOGS_OK, "lobby join"); //REMOVE
        } break;
        case EVENT_TYPE_LOBBY_LEAVE: {
            //TODO
            mirabel_slogf(LOGS_OK, "lobby leave"); //REMOVE
        } break;
        case EVENT_TYPE_LOBBY_CDJL_ERR: {
            //TODO
            mirabel_slogf(LOGS_OK, "lobby err: %s", e->lobby_cdjl_err.err_msg); //REMOVE
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

void workspace_lobby_create(workspace* self)
{
    event_any e;
    event_create_lobby_create(&e, self->lobby_name, self->lobby_password);
    workspace_netc_send(self, &e);
}

void workspace_lobby_destroy(workspace* self)
{
    event_any e;
    event_create_lobby_base(&e, EVENT_TYPE_LOBBY_DESTROY, self->lobby_id);
    workspace_netc_send(self, &e);
}

void workspace_lobby_join(workspace* self)
{
    event_any e;
    event_create_lobby_join(&e, self->lobby_name, self->lobby_password, EVENT_LOBBY_NONE);
    workspace_netc_send(self, &e);
}

void workspace_lobby_leave(workspace* self)
{
    event_any e;
    event_create_lobby_base(&e, EVENT_TYPE_LOBBY_LEAVE, self->lobby_id);
    workspace_netc_send(self, &e);
}
