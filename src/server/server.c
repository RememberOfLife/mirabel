#include "mirabel/server.h"

bool server_create(server* srv, bool offline)
{
    srv->offline = offline;
    event_queue_create(&srv->inbox);
    return false;
}

void server_destroy(server* srv)
{
    event_queue_destroy(&srv->inbox);
}

bool server_update(server* srv)
{
    bool exit = false;
    uint32_t remaining_budget = 1024; // limit maximum event processing if queue is too big
    while (remaining_budget > 0) {
        event_any e;
        event_queue_pop(&srv->inbox, &e, UINT32_MAX);
        switch (e.base.type) {
            case EVENT_TYPE_NULL: {
                mirabel_slogf(LOGS_LESS, "server: received unexpected null event\n");
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
