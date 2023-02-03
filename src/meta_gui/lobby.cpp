#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void lobby_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(450, 550), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Lobby Config", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        //TODO

        ImGui::End();
    }

} // namespace MetaGui
