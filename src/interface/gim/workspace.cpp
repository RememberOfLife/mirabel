#include <cstdint>

#include "mirabel/application.h"
#include "mirabel/client.h"

#include "interface/gim/workspace.hpp"

gim_workspace::gim_workspace(uint32_t workspace_id)
{
    client_workspace = client_get_workspace_by_id(appi.aclient, workspace_id);
}

void gim_workspace::connection_new()
{
    client_workspace->netc = client_add_connection(appi.aclient);
}

void gim_workspace::connection_attach(network_connection* net_conn)
{
    client_workspace->netc = net_conn;
}

void gim_workspace::connection_detach()
{
    //TODO need to do cleanup and possibly gracefully disconnect our workspace here..
    client_workspace->netc = NULL;
}
