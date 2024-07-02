#include <stdbool.h>

#include "mirabel/event_queue.h"
#include "mirabel/network_adapter.h"

/////
// public

const char* network_adapter_get_last_error(network_adapter* self)
{
    return self->methods->get_last_error(self);
}

bool network_adapter_create(network_adapter* self)
{
    event_queue_create(&self->outbox);
    return self->methods->create(self);
}

void network_adapter_destroy(network_adapter* self)
{
    self->methods->destroy(self);
    event_queue_destroy(&self->outbox);
}
