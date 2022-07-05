#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void confirm_exit_modal(bool* p_open)
    {
        if (*p_open) {
            ImGui::OpenPopup("Exit?");
        }
        // always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Exit?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("A game is running, do you really want to exit?");
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
            if (ImGui::Button("EXIT")) {
                f_event_any es;
                f_event_create_type(&es, EVENT_TYPE_EXIT);
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
            ImGui::PopStyleColor(3);
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(-1.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
                show_confirm_exit_modal = false;
            }
            ImGui::EndPopup();
        }
    }

}
