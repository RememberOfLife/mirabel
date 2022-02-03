#include <cstdint>

#include "imgui.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void guistate_config_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Guistate Config", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }
        
        ImGui::End();
    }

}
