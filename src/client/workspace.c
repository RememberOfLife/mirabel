#include <stdbool.h>

#include "mirabel/event_queue.h"

#include "mirabel/workspace.h"

bool workspace_create(workspace* ws)
{
    ws->netc = NULL;
    event_queue_create(&ws->net_inbox);
    event_queue_create(&ws->ifc_inbox);
    return false;
}

void workspace_destroy(workspace* ws)
{
    event_queue_destroy(&ws->net_inbox);
    event_queue_destroy(&ws->ifc_inbox);
}
