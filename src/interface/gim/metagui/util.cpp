#include "imgui.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_util_push_button_colors(METAGUI_UTIL_BUTTON_TYPE btn_type)
{
    switch (btn_type) {
        case METAGUI_UTIL_BUTTON_TYPE_SUCCESS: {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(58, 154, 58, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(81, 212, 81, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(51, 226, 51, 255));
        } break;
        case METAGUI_UTIL_BUTTON_TYPE_WARN: {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(218, 176, 41, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(238, 196, 56, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(240, 170, 0, 255));
        } break;
        case METAGUI_UTIL_BUTTON_TYPE_DANGER: {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
        } break;
        default: {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            assert(0);
        } break;
    }
}

void graphical_immediate_mode_interface::metagui_util_pop_button_colors()
{
    ImGui::PopStyleColor(3);
}
