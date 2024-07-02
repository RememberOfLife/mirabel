#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/workspace.h"

#include "mirabel/client.h"

bool client_create(client* clt)
{
    VEC_CREATE(&clt->net_conns, 2);
    VEC_CREATE(&clt->workspaces, 1);
    return false;
}

void client_destroy(client* clt)
{
    for (size_t i = 0; i < VEC_LEN(&clt->net_conns); i++) {
        //TODO destroy network connection
    }
    VEC_DESTROY(&clt->net_conns);
    for (size_t i = 0; i < VEC_LEN(&clt->workspaces); i++) {
        workspace_destroy(clt->workspaces[i]);
    }
    VEC_DESTROY(&clt->workspaces);
}

bool client_update(client* clt)
{
    return false;
}
