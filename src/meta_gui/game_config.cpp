#include <cstdint>

#include "imgui.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "games/game_catalogue.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t game_base_idx = 0;
    uint32_t game_variant_idx = 0;
    uint32_t game_impl_idx = 0;
    void* game_load_options = NULL;
    void* game_runtime_options = NULL;

    void game_config_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 80), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Game Config", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        Control::PluginManager& plugin_mgr = Control::main_client->plugin_mgr;

        //BUG same problem as engines, what happens if impl gets unloaded while its runtime options have already been created, maybe do need a ref count

        const char* selected_game_name = "<NONE>";
        const char* selected_variant_name = "<NONE>";
        const char* selected_impl_name = "<NONE>";
        // invalidate game base/variant/impl and set to default
        if (game_base_idx > 0) {
            if (plugin_mgr.game_lookup.find(game_base_idx) == plugin_mgr.game_lookup.end()) {
                // base game catalogue idx has been invalidated, reset all to NONE
                game_base_idx = 0;
                game_variant_idx = 0;
                if (game_impl_idx > 0) {
                    plugin_mgr.impl_lookup[game_impl_idx]->destroy_opts(game_load_options);
                    game_load_options = NULL;
                }
                game_impl_idx = 0;
            } else {
                selected_game_name = plugin_mgr.game_lookup[game_base_idx]->name.c_str();
            }
        }

        bool game_running = (Control::main_client->the_game != NULL);
        // draw game start,stop,restart
        // locks all pre loading input elements if game is running, stop is only available if running
        bool disable_startstop = (game_base_idx == 0);
        if (disable_startstop) {
            ImGui::BeginDisabled();
        }
        if (game_running) {
            if (ImGui::Button("Restart")) {
                event_any es;
                event_create_game_load(&es, plugin_mgr.game_lookup[game_base_idx]->name.c_str(), plugin_mgr.variant_lookup[game_variant_idx]->name.c_str(), plugin_mgr.impl_lookup[game_impl_idx]->get_name(), NULL);
                event_queue_push(&Control::main_client->inbox, &es);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                event_any es;
                event_create_type(&es, EVENT_TYPE_GAME_UNLOAD);
                event_queue_push(&Control::main_client->inbox, &es);
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                event_any es;
                event_create_game_load(&es, plugin_mgr.game_lookup[game_base_idx]->name.c_str(), plugin_mgr.variant_lookup[game_variant_idx]->name.c_str(), plugin_mgr.impl_lookup[game_impl_idx]->get_name(), NULL);
                event_queue_push(&Control::main_client->inbox, &es);
            }
        }
        if (disable_startstop) {
            ImGui::EndDisabled();
        }

        bool disable_game_selection = (plugin_mgr.game_catalogue.size() == 0);
        if (game_running || disable_game_selection) {
            ImGui::BeginDisabled();
        }
        // draw base game combo box
        if (ImGui::BeginCombo("Game", selected_game_name, ImGuiComboFlags_None)) {
            // offer idx 0 element, so selection can be reset
            bool is_selected = (game_base_idx == 0);
            if (ImGui::Selectable("<NONE>", is_selected)) {
                game_base_idx = 0;
                game_variant_idx = 0;
                if (game_impl_idx > 0) {
                    plugin_mgr.impl_lookup[game_impl_idx]->destroy_opts(game_load_options);
                    game_load_options = NULL;
                }
                game_impl_idx = 0;
            }
            // set the initial focus when opening the combo
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
            for (Control::BaseGame it : plugin_mgr.game_catalogue) {
                is_selected = (game_base_idx == it.map_idx);
                if (ImGui::Selectable(it.name.c_str(), is_selected)) {
                    game_base_idx = it.map_idx;
                    game_variant_idx = 0;
                    if (game_impl_idx > 0) {
                        plugin_mgr.impl_lookup[game_impl_idx]->destroy_opts(game_load_options);
                        game_load_options = NULL;
                    }
                    game_impl_idx = 0;
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (game_base_idx > 0) {
            // game selected, make sure a variant is selected aswell
            if (plugin_mgr.variant_lookup.find(game_variant_idx) == plugin_mgr.variant_lookup.end()) {
                // variant invalidated, set to any valid one
                game_variant_idx = plugin_mgr.game_lookup[game_base_idx]->variants.begin()->map_idx;
            }
            selected_variant_name = plugin_mgr.variant_lookup[game_variant_idx]->name.c_str();
        }
        // draw base game variant combo box, disabled if only one option or if variant idx 0
        bool disable_variant_selection = (game_variant_idx == 0 || plugin_mgr.game_lookup[game_base_idx]->variants.size() == 1);
        if (disable_variant_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Variant", selected_variant_name, disable_variant_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (Control::BaseGameVariant it : plugin_mgr.game_lookup[game_base_idx]->variants) {
                bool is_selected = (game_variant_idx == it.map_idx);
                if (ImGui::Selectable(it.name.c_str(), is_selected)) {
                    game_variant_idx = it.map_idx;
                    if (game_impl_idx > 0) {
                        plugin_mgr.impl_lookup[game_impl_idx]->destroy_opts(game_load_options);
                        game_load_options = NULL;
                    }
                    game_impl_idx = 0;
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (disable_variant_selection) {
            ImGui::EndDisabled();
        }

        if (game_variant_idx > 0) {
            // variant selected, make sure an impl is selected aswell
            if (plugin_mgr.impl_lookup.find(game_impl_idx) == plugin_mgr.impl_lookup.end()) {
                // impl invalidated, set to any valid one
                game_impl_idx = plugin_mgr.variant_lookup[game_variant_idx]->impls.begin()->map_idx;
                plugin_mgr.impl_lookup[game_impl_idx]->create_opts(&game_load_options);
            }
            selected_impl_name = plugin_mgr.impl_lookup[game_impl_idx]->get_name();
        }
        // draw impl selection combo box, disabled if only one option if impl idx 0
        bool disable_impl_selection = (game_impl_idx == 0 || plugin_mgr.variant_lookup[game_variant_idx]->impls.size() == 1);
        if (disable_impl_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Impl", selected_impl_name, disable_impl_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (Control::BaseGameVariantImpl it : plugin_mgr.variant_lookup[game_variant_idx]->impls) {
                bool is_selected = (game_impl_idx == it.map_idx);
                if (ImGui::Selectable(it.get_name(), is_selected)) {
                    if (game_impl_idx > 0) {
                        plugin_mgr.impl_lookup[game_impl_idx]->destroy_opts(game_load_options);
                        game_load_options = NULL;
                    }
                    game_impl_idx = it.map_idx;
                    plugin_mgr.impl_lookup[game_impl_idx]->create_opts(&game_load_options);
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (disable_impl_selection) {
            ImGui::EndDisabled();
        }
        if (game_running || disable_game_selection) {
            ImGui::EndDisabled();
        }

        ImGui::Separator();

        // draw options panel
        if (ImGui::CollapsingHeader(game_running ? "Options [locked]" : "Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (game_running) {
                ImGui::BeginDisabled();
            }
            if (game_load_options) {
                plugin_mgr.impl_lookup[game_impl_idx]->display_opts(game_load_options);
            } else {
                ImGui::TextDisabled("<no options>");
            }
            if (game_running) {
                ImGui::EndDisabled();
            }
        }

        ImGui::Separator();

        // draw internal state editor, only if a game is running
        if (game_running) {
            if (ImGui::CollapsingHeader("State Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (game_runtime_options) {
                    plugin_mgr.impl_lookup[game_impl_idx]->display_runtime(Control::main_client->the_game, game_runtime_options);
                } else {
                    ImGui::TextDisabled("<no runtime>");
                }
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::CollapsingHeader("State Editor", ImGuiTreeNodeFlags_Leaf);
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

} // namespace MetaGui
