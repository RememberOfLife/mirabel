#include <cstdint>
#include <cstdio>

#include "imgui.h"
#include "rosalia/vector.h"

#include "mirabel/server/lobby_manager.h"
#include "mirabel/server/user_manager.h"
#include "mirabel/application.h"
#include "mirabel/server.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_server()
{
    if (!show_server) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    bool window_contents_visible = ImGui::Begin("Server Internals", &show_server);
    if (!window_contents_visible) {
        ImGui::End();
        return;
    }
    /////
    // server imgui window content
    server* srv = appi.aserver;
    if (srv == NULL) {
        ImGui::TextUnformatted("no server available");
        ImGui::End();
        return;
    }

    if (srv->offline) {
        ImGui::Text("Offline");
    } else {
        ImGui::Text("Online");
    }

    if (ImGui::CollapsingHeader("Network Adapters")) {
        //TODO make this a table
        for (size_t neta_idx = 0; neta_idx < VEC_LEN(&srv->netas); neta_idx++) {
            ImGui::Text("#%zu ^%p: %s", neta_idx, srv->netas[neta_idx], srv->netas[neta_idx]->methods->name);
        }
    }

    if (ImGui::CollapsingHeader("Client Connections")) {
        ImGui::Text("total connections: %zu", VEC_LEN(&srv->connections) - 1);
        ImGui::Separator();
        size_t reuse_slots = 0;
        for (size_t conn_idx = 1; conn_idx < VEC_LEN(&srv->connections); conn_idx++) {
            if (srv->connections[conn_idx].responsible_neta == NULL) {
                reuse_slots++;
                continue;
            }
            ImGui::Text("#%zu: rneta^%p + adapter_connid[%u]", conn_idx, srv->connections[conn_idx].responsible_neta, srv->connections[conn_idx].neta_local_connection_id);
        }
        ImGui::Separator();
        ImGui::Text("%zu reusable slots", reuse_slots);
    }

    if (ImGui::CollapsingHeader("Workspace Handles")) {
        ImGui::Text("total handles: %zu", VEC_LEN(&srv->workspaces) - 1);
        ImGui::Separator();
        size_t reuse_slots = 0;
        for (size_t wsh_idx = 1; wsh_idx < VEC_LEN(&srv->workspaces); wsh_idx++) {
            if (srv->workspaces[wsh_idx].connection_id == EVENT_CONNECTION_NONE) {
                reuse_slots++;
                continue;
            }
            ImGui::Text("#%zu: conn[%u] + client_wsidx[%u] + lobby[%u]", wsh_idx, srv->workspaces[wsh_idx].connection_id, srv->workspaces[wsh_idx].client_local_workspace_id, srv->workspaces[wsh_idx].lobby_id);
        }
        ImGui::Separator();
        ImGui::Text("%zu reusable slots", reuse_slots);
    }

    if (ImGui::CollapsingHeader("User Manager")) {
        ImGui::Text("loaded users: %zu", VEC_LEN(&srv->user_mgr.loaded_slots));
        ImGui::Separator();
        for (size_t user_idx = 0; user_idx < VEC_LEN(&srv->user_mgr.loaded_slots); user_idx++) {
            ImGui::Text("#%zu: %c name\"%s\"", user_idx, srv->user_mgr.loaded_slots[user_idx].is_guest ? '~' : '|', srv->user_mgr.loaded_slots[user_idx].username);
        }
    }

    if (ImGui::CollapsingHeader("Lobby Manager")) {
        ImGui::TextUnformatted("<TODO>");
    }

    // end of window
    /////
    ImGui::End();
}
