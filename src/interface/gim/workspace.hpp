#pragma once

#include <cstdint>

#include "mirabel/workspace.h"

struct gim_workspace {
    workspace* client_workspace;

    size_t current_connection_idx = 0;

    gim_workspace(uint32_t workspace_id);

    void connection_new();
    void connection_attach(network_connection* net_conn);
    void connection_detach();
};
