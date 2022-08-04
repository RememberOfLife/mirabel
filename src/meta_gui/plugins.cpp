#include <cstdint>
#include <vector>

#include "imgui.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/client.hpp"
#include "control/plugins.hpp"
#include "frontends/empty_frontend.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void plugins_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Plugins", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        std::vector<Control::PluginManager::plugin_file>& plugins_ref = Control::main_client->plugin_mgr.plugins;

        if (ImGui::Button("refresh files")) {
            Control::main_client->plugin_mgr.detect_plugins();
        }
        ImGui::SameLine();
        ImGui::Text("%lu files detected", plugins_ref.size());

        const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
        if (ImGui::BeginTable("plugins_table", 2, flags))
        {
            ImGui::TableSetupColumn("  ", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Pluginpath");
            ImGui::TableHeadersRow();

            for (int i = 0; i < plugins_ref.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::PushID(i);
                ImGui::TableSetColumnIndex(0);
                if (!plugins_ref[i].loaded) {
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.5f, 0.6f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.5f, 0.7f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.5f, 0.8f, 0.8f));
                    if (ImGui::Button("+")) {
                        Control::main_client->plugin_mgr.load_plugin(i);
                    }
                    ImGui::PopStyleColor(3);
                } else {
                    bool unload_disabled = (Control::main_client->the_game != NULL || Control::main_client->engine_mgr->engines.size() > 0 || dynamic_cast<Frontends::EmptyFrontend*>(Control::main_client->frontend) == nullptr);
                    if (unload_disabled) {
                        ImGui::BeginDisabled();
                    }
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                    if (ImGui::Button("-")) {
                        Control::main_client->plugin_mgr.unload_plugin(i);
                    }
                    ImGui::PopStyleColor(3);
                    if (unload_disabled) {
                        ImGui::EndDisabled();
                    }
                }
                ImGui::TableSetColumnIndex(1);
                //TODO setup right click for whole name line to see a detailed view of what a plugin provides
                ImGui::Text("%s", plugins_ref[i].filename.c_str());
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

}
