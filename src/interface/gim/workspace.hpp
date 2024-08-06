#pragma once

#include <cstdint>

#include "mirabel/workspace.h"

struct gim_workspace {
    workspace* client_workspace;

    //TODO any way to remove this from here and not have to keep it
    size_t current_connection_idx;

    gim_workspace(uint32_t workspace_id);

    void connection_new();
    void connection_attach(network_connection* net_conn);
    void connection_detach();
};
