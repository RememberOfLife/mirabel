#include "mirabel/server.h"

bool server_create(server* srv, bool offline)
{
    srv->offline = offline;
    return false;
}

void server_destroy(server* srv)
{
}

bool server_update(server* srv)
{
    return false;
}
