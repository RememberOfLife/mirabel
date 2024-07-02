#include <stdbool.h>

#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"
#include "mirabel/network_connection.h"

bool network_connection_create(network_connection* self)
{
    //TODO
    event_queue_create(&self->inbox);
    self->neta.inbox = &self->inbox;
    self->outbox = &self->neta.outbox;
    //TODO
}

void network_connection_destroy(network_connection* self)
{
    //TODO
    event_queue_destroy(&self->inbox);
    //TODO
}
