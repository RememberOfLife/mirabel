#include <stdbool.h>

#include "mirabel/workspace.h"

bool workspace_create(workspace* self)
{
    self->netc = NULL;
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
