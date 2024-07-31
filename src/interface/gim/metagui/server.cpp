#include <cstdint>
#include <cstdio>

#include "imgui.h"

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

    //TODO display server internals, e.g. where it listens on, users, lobbies, etc..

    ImGui::End();
}
