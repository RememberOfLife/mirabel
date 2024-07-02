#include <stdbool.h>

#include "rosalia/vector.h"

#include "mirabel/workspace.h"

#include "mirabel/client.h"

bool client_create(client* self)
{
    VEC_CREATE(&self->net_conns, 2);
    VEC_CREATE(&self->workspaces, 1);
    return false;
}

void client_destroy(client* self)
{
    for (size_t i = 0; i < VEC_LEN(&self->net_conns); i++) {
        //TODO destroy network connection
    }
    VEC_DESTROY(&self->net_conns);
    for (size_t i = 0; i < VEC_LEN(&self->workspaces); i++) {
        workspace_destroy(self->workspaces[i]);
    }
    VEC_DESTROY(&self->workspaces);
}

bool client_update(client* self)
{
    return false;
}
