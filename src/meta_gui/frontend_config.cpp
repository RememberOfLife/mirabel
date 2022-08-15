#include <cstdint>
#include <set>

#include "imgui.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/frontend.h"
#include "control/client.hpp"
#include "control/plugins.hpp"
#include "frontends/frontend_catalogue.hpp"
#include "games/game_catalogue.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t fe_selection_idx = UINT32_MAX; // selector inside the fe combo box
    uint32_t selected_fem_idx = 0; // map lookup idx for the selected fe
    uint32_t running_fem_idx = 0;
    void* frontend_load_options = NULL;

    void frontend_config_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(300, 80), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Frontend Config", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        Control::PluginManager& plugin_mgr = Control::main_client->plugin_mgr;

        // collect all compatible frontends
        //TODO cache this together with the game_impl_idx to which they belong, and update aswell if new thigns loaded
        const game_methods* running_methods = NULL;
        if (plugin_mgr.impl_lookup.find(game_impl_idx) != plugin_mgr.impl_lookup.end()) {
            running_methods = plugin_mgr.impl_lookup[game_impl_idx]->get_methods();
        }
        std::vector<uint32_t> compatible_fem{};
        bool selected_fem_compatible = false;
        for (Control::FrontendImpl fe : plugin_mgr.frontend_catalogue) {
            if (running_methods && fe.methods->is_game_compatible(running_methods) == ERR_OK) {
                compatible_fem.emplace_back(fe.map_idx);
                if (fe.map_idx == selected_fem_idx) {
                    selected_fem_compatible = true;
                    fe_selection_idx = compatible_fem.size() - 1;
                }
            }
        }

        // if a frontend is running, which is not the empty frontend, and not compatible, unload it
        if (running_fem_idx > 0 && !selected_fem_compatible && selected_fem_idx > 0) {
            plugin_mgr.frontend_lookup[selected_fem_idx]->destroy_opts(frontend_load_options);
            selected_fem_idx = 0;
            f_event_any es;
            f_event_create_type(&es, EVENT_TYPE_FRONTEND_UNLOAD);
            f_event_queue_push(&Control::main_client->inbox, &es);
        }
        // if the selected frontend is no longer compatible, deselect
        if (!selected_fem_compatible) {
            //TODO mabye select any one of the available ones?
            fe_selection_idx = UINT32_MAX;
            if (selected_fem_idx > 0) {
                plugin_mgr.frontend_lookup[selected_fem_idx]->destroy_opts(frontend_load_options);
            }
            selected_fem_idx = 0;
        }

        bool fronend_running = (running_fem_idx > 0);
        // draw game start,stop,restart
        // locks all pre loading input elements if frontend is running, stop is only available if running
        bool disable_fronend_loader = (selected_fem_idx == 0);
        if (disable_fronend_loader) {
            ImGui::BeginDisabled();
        }
        if (fronend_running) {
            if (ImGui::Button("Restart")) {
                f_event_any es;
                f_event_create_frontend_load(&es, plugin_mgr.frontend_lookup[selected_fem_idx]->new_frontend(&Control::main_client->dd, frontend_load_options));
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                f_event_any es;
                f_event_create_type(&es, EVENT_TYPE_FRONTEND_UNLOAD);
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                f_event_any es;
                f_event_create_frontend_load(&es, plugin_mgr.frontend_lookup[selected_fem_idx]->new_frontend(&Control::main_client->dd, frontend_load_options));
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
        }
        if (disable_fronend_loader) {
            ImGui::EndDisabled();
        }
        // draw frontend_wrap combo box, show only compatible ones
        bool disable_frontend_selection = (compatible_fem.size() < 1);
        if (compatible_fem.size() == 0) {
            fe_selection_idx = UINT32_MAX;
            if (selected_fem_idx > 0) {
                plugin_mgr.frontend_lookup[selected_fem_idx]->destroy_opts(frontend_load_options);
            }
            selected_fem_idx = 0;
        }
        if (fronend_running || disable_frontend_selection) {
            ImGui::BeginDisabled();
        }
        const char* selected_fem_name = "<empty>";
        if (selected_fem_idx > 0) {
            if (plugin_mgr.frontend_lookup.find(selected_fem_idx) == plugin_mgr.frontend_lookup.end()) {
                // selected frontend invalid, select any compatible one and create opts
                //TODO
            }
            selected_fem_name = plugin_mgr.frontend_lookup[selected_fem_idx]->get_name();
        }
        if (ImGui::BeginCombo("Frontend", selected_fem_name, disable_frontend_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            // offer idx 0 element, so selection can be reset
            bool is_selected = (fe_selection_idx == UINT32_MAX);
            if (ImGui::Selectable("<empty>", is_selected)) {
                fe_selection_idx = UINT32_MAX;
                if (selected_fem_idx > 0) {
                    plugin_mgr.frontend_lookup[selected_fem_idx]->destroy_opts(frontend_load_options);
                }
                selected_fem_idx = 0;
            }
            // set the initial focus when opening the combo
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
            for (int fei = 0; fei < compatible_fem.size(); fei++) {
                uint32_t fe_map_idx = compatible_fem[fei];
                bool is_selected = (fei == fe_selection_idx);
                if (ImGui::Selectable(plugin_mgr.frontend_lookup[fe_map_idx]->get_name(), is_selected)) {
                    fe_selection_idx = fei;
                    if (selected_fem_idx > 0) {
                        plugin_mgr.frontend_lookup[selected_fem_idx]->destroy_opts(frontend_load_options);
                    }
                    selected_fem_idx = fe_map_idx;
                    plugin_mgr.frontend_lookup[selected_fem_idx]->create_opts(&frontend_load_options);
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (fronend_running || disable_frontend_selection) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader(fronend_running ? "Options [locked]" : "Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (fronend_running) {
                ImGui::BeginDisabled();
            }
            if (frontend_load_options) {
                plugin_mgr.frontend_lookup[selected_fem_idx]->display_opts(frontend_load_options);
            } else {
                ImGui::TextDisabled("<no options>");
            }
            if (fronend_running) {
                ImGui::EndDisabled();
            }
        }
        ImGui::Separator();
        if (fronend_running) {
            if (ImGui::CollapsingHeader("Graphics Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                Control::main_client->the_frontend->methods->runtime_opts_display(Control::main_client->the_frontend);
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::CollapsingHeader("Graphics Options", ImGuiTreeNodeFlags_Leaf);
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

}
