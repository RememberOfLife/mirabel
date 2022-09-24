#include <cstdint>

#include "imgui_internal.h" // for ImGuiDockNode
#include "imgui.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void global_dockspace(float* x, float* y, float* w, float* h)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        dockspace_flags |= ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_None;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        host_window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
        host_window_flags |= ImGuiWindowFlags_NoBackground;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("global_dockspace_window", NULL, host_window_flags);
        ImGui::PopStyleVar(3);
        ImGuiID dockspace_id = ImGui::GetID("global_dockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        ImGuiDockNode* dn = ImGui::DockBuilderGetNode(dockspace_id);
        *x = dn->CentralNode->Pos.x;
        *y = dn->CentralNode->Pos.y;
        *w = dn->CentralNode->Size.x;
        *h = dn->CentralNode->Size.y;
        ImGui::End();
    }

} // namespace MetaGui
