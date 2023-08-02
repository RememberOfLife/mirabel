#include <cstdint>
#include <set>
#include <vector>

#include "imgui.h"
#include "mirabel/engine.h"
#include "mirabel/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"
#include "control/plugins.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void plugins_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Plugins", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        Control::PluginManager& plugin_mgr = Control::main_client->plugin_mgr;

        if (ImGui::BeginTabBar("plugins and components")) {
            if (ImGui::BeginTabItem("Plugins")) {

                std::vector<Control::PluginManager::plugin_file>& plugins_ref = plugin_mgr.plugins;

                if (ImGui::Button("refresh files")) {
                    plugin_mgr.detect_plugins();
                }
                ImGui::SameLine();
                ImGui::Text("%lu files detected", plugins_ref.size());

                const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
                if (ImGui::BeginTable("plugins_table", 2, table_flags)) {
                    ImGui::TableSetupColumn("  ", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Pluginpath");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < plugins_ref.size(); i++) {
                        ImGui::TableNextRow();
                        ImGui::PushID(i);
                        ImGui::TableSetColumnIndex(0);
                        if (!plugins_ref[i].loaded) {
                            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.5f, 0.6f, 0.6f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.5f, 0.7f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.5f, 0.8f, 0.8f));
                            if (ImGui::Button("+")) {
                                plugin_mgr.load_plugin(i);
                            }
                            ImGui::PopStyleColor(3);
                        } else {
                            bool unload_disabled = (Control::main_client->the_game != NULL || /* //TODO REENABLE engine Control::main_client->engine_mgr->engines.size() > 0 ||*/ Control::main_client->the_frontend != Control::main_client->empty_fe || MetaGui::game_base_idx > 0 || MetaGui::fe_selection_idx != UINT32_MAX);
                            if (unload_disabled) {
                                ImGui::BeginDisabled();
                            }
                            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                            if (ImGui::Button("-")) {
                                plugin_mgr.unload_plugin(i);
                            }
                            ImGui::PopStyleColor(3);
                            if (unload_disabled) {
                                ImGui::EndDisabled();
                            }
                        }
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", plugins_ref[i].filename.c_str());
                        //TODO setup right click for whole name line to see a detailed view of what a plugin provides
                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Catalogues")) {

                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f); // push slightly to the right
                    ImGui::TextUnformatted("W = Wrapped, can be True/False");
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }

                const ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

                if (ImGui::BeginTable("game_catalogue_table", 5, table_flags)) {
                    ImGui::TableSetupColumn("Game");
                    ImGui::TableSetupColumn("Variant");
                    ImGui::TableSetupColumn("Impl");
                    ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();
                    for (Control::BaseGame itrG : plugin_mgr.game_catalogue) {
                        for (Control::BaseGameVariant itrV : itrG.variants) {
                            for (Control::BaseGameVariantImpl itrI : itrV.impls) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%s", itrG.name.c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", itrV.name.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", itrI.get_name());
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%u.%u.%u", itrI.get_methods()->version.major, itrI.get_methods()->version.minor, itrI.get_methods()->version.patch);
                                ImGui::TableSetColumnIndex(4);
                                if (itrI.wrapped) {
                                    ImGui::Text("T");
                                } else {
                                    ImGui::TextDisabled("F");
                                }
                            }
                        }
                    }
                    ImGui::EndTable();
                }

                if (ImGui::BeginTable("frontend_catalogue_table", 2, table_flags)) {
                    ImGui::TableSetupColumn("Frontend");
                    ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();
                    for (Control::FrontendImpl itrF : plugin_mgr.frontend_catalogue) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", itrF.get_name());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u.%u.%u", itrF.methods->version.major, itrF.methods->version.minor, itrF.methods->version.patch);
                    }
                    ImGui::EndTable();
                }

                if (ImGui::BeginTable("engine_catalogue_table", 3, table_flags)) {
                    ImGui::TableSetupColumn("Engine");
                    ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();
                    for (Control::EngineImpl itrE : plugin_mgr.engine_catalogue) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", itrE.get_name());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u.%u.%u", itrE.get_methods()->version.major, itrE.get_methods()->version.minor, itrE.get_methods()->version.patch);
                        ImGui::TableSetColumnIndex(2);
                        if (itrE.wrapped) {
                            ImGui::Text("T");
                        } else {
                            ImGui::TextDisabled("F");
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

} // namespace MetaGui
