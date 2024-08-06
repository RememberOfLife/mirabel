#include <cstdint>

#include "mirabel/application.h"
#include "mirabel/client.h"

#include "interface/gim/workspace.hpp"

gim_workspace::gim_workspace(uint32_t workspace_id)
{
    client_workspace = client_get_workspace_by_id(appi.aclient, workspace_id);
    current_connection_idx = 0;
}

void gim_workspace::connection_new()
{
    workspace_connection_attach(client_workspace, client_add_connection(appi.aclient));
}

void gim_workspace::connection_attach(network_connection* net_conn)
{
    workspace_connection_attach(client_workspace, net_conn);
}

void gim_workspace::connection_detach()
{
    //TODO need to do cleanup and possibly gracefully disconnect our workspace here..
    workspace_connection_detach(client_workspace);
    current_connection_idx = 0;
}
