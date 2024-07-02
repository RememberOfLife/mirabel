#include "mirabel/server.h"

bool server_create(server* self, bool offline)
{
    self->offline = offline;
    event_queue_create(&self->inbox);
    return false;
}

void server_destroy(server* self)
{
    event_queue_destroy(&self->inbox);
}

bool server_update(server* self)
{
    bool exit = false;
    int32_t remaining_budget = 1024; // limit maximum event processing if queue is too big
    while (remaining_budget > 0) {
        remaining_budget--;
        event_any e;
        event_queue_pop(&self->inbox, &e, 0); //TODO for a true ONLY server, we will end spinning a lot if we do this, also the crl interface is blocking..
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
                mirabel_slogf(e.log.status, "server: queue log: %s", e.log.str);
            } break;
            //TODO other event types
            default: {
                mirabel_slogf(LOGS_WARN, "server: received unexpected event, type: %d %s\n", e.base.type, event_type_str(e.base.type));
            } break;
        }
        event_destroy(&e);
    }
    return exit;
}
