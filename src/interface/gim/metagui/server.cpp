#include <cstdint>
#include <cstdio>

#include "imgui.h"
#include "rosalia/vector.h"

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
            ImGui::Text("#%zu (%p): %s", neta_idx, srv->netas[neta_idx], srv->netas[neta_idx]->methods->name);
        }
    }

    if (ImGui::CollapsingHeader("Client Connections")) {
        ImGui::Text("total connections: %zu", VEC_LEN(&srv->connections) - 1);
        ImGui::Separator();
        uint32_t reuse_slots = 0;
        for (size_t conn_idx = 1; conn_idx < VEC_LEN(&srv->connections); conn_idx++) {
            if (srv->connections[conn_idx].responsible_neta == NULL) {
                reuse_slots++;
                continue;
            }
            ImGui::Text("#%zu: %p + %u", conn_idx, srv->connections[conn_idx].responsible_neta, srv->connections[conn_idx].neta_local_connection_id);
        }
        ImGui::Separator();
        ImGui::Text("%u reusable slots", reuse_slots);
    }

    //TODO display server internals, e.g. where it listens on, users, lobbies, etc..

    // end of window
    /////
    ImGui::End();
}
