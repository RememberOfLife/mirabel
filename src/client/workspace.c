#include <stdbool.h>
#include <stdint.h>

#include "mirabel/event.h"

#include "mirabel/workspace.h"

uint32_t next_workspace_id = 1;

bool workspace_create(workspace* self)
{
    self->netc = NULL;
    self->id = next_workspace_id++;
    self->server_workspace_created = false;
    return false;
}

void workspace_destroy(workspace* self)
{
    //TODO if wanted, notify client that network connection has one less user so maybe it can be shutdown
}

void workspace_process_network_event(workspace* self, event_any* e)
{
    //TODO
}
