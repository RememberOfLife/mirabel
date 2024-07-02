#include <stdbool.h>

#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"
#include "mirabel/network_connection.h"

bool network_connection_create(network_connection* netc)
{
    //TODO
    event_queue_create(&netc->inbox);
    netc->neta.inbox = &netc->inbox;
    netc->outbox = &netc->neta.outbox;
    //TODO
}

void network_connection_destroy(network_connection* netc)
{
    //TODO
    event_queue_destroy(&netc->inbox);
    //TODO
}
