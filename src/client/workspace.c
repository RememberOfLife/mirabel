#include <stdbool.h>

#include "mirabel/event_queue.h"

#include "mirabel/workspace.h"

bool workspace_create(workspace* self)
{
    self->netc = NULL;
    event_queue_create(&self->net_inbox);
    event_queue_create(&self->ifc_inbox);
    return false;
}

void workspace_destroy(workspace* self)
{
    event_queue_destroy(&self->net_inbox);
    event_queue_destroy(&self->ifc_inbox);
}
