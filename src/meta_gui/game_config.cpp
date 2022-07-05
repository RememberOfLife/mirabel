#include <cstdint>

#include "imgui.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "games/game_catalogue.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t base_game_idx = 0;
    uint32_t game_variant_idx = 0;

    void game_config_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 80), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Game Config", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }
        bool game_running = (Control::main_client->the_game != NULL);
        // draw game start,stop,restart
        // locks all pre loading input elements if game is running, stop is only available if running
        //HACK options are currently still just buffered by the base_game_variant class that provides the new game, this will not work for network loads, because they do not have options set yet
        if (game_running) {
            if (ImGui::Button("Restart")) {
                f_event_any es;
                f_event_create_game_load(&es, Games::game_catalogue[base_game_idx].name, Games::game_catalogue[base_game_idx].variants[game_variant_idx]->name, NULL);
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                f_event_any es;
                f_event_create_type(&es, EVENT_TYPE_GAME_UNLOAD);
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                f_event_any es;
                f_event_create_game_load(&es, Games::game_catalogue[base_game_idx].name, Games::game_catalogue[base_game_idx].variants[game_variant_idx]->name, NULL);
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
        }
        if (game_running) {
            ImGui::BeginDisabled();
        }
        // draw base game combo box
        if (ImGui::BeginCombo("Game", Games::game_catalogue[base_game_idx].name, ImGuiComboFlags_None)) {
            for (int i = 0; i < Games::game_catalogue.size(); i++) {
                bool is_selected = (base_game_idx == i);
                if (ImGui::Selectable(Games::game_catalogue[i].name, is_selected)) {
                    base_game_idx = i;
                    game_variant_idx = 0; // reset selected variant when switching base game
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        // draw base game variant combo box, disabled if only one option
        bool disable_variant_selection = (Games::game_catalogue[base_game_idx].variants.size() == 1);
        if (disable_variant_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Variant", Games::game_catalogue[base_game_idx].variants[game_variant_idx]->name, disable_variant_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (int i = 0; i < Games::game_catalogue[base_game_idx].variants.size(); i++) {
                bool is_selected = (game_variant_idx == i);
                if (ImGui::Selectable(Games::game_catalogue[base_game_idx].variants[i]->name, is_selected)) {
                    game_variant_idx = i;
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
        if (game_running) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        // draw options panel
        if (ImGui::CollapsingHeader(game_running ? "Options [locked]" : "Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (game_running) {
                ImGui::BeginDisabled();
            }
            Games::game_catalogue[base_game_idx].variants[game_variant_idx]->draw_options();
            if (game_running) {
                ImGui::EndDisabled();
            }
        }
        ImGui::Separator();
        // draw internal state editor, only if a game is running
        if (game_running) {
            if (ImGui::CollapsingHeader("State Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
                Games::game_catalogue[base_game_idx].variants[game_variant_idx]->draw_state_editor(Control::main_client->the_game);
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::CollapsingHeader("State Editor", ImGuiTreeNodeFlags_Leaf);
            ImGui::EndDisabled();
        }
        ImGui::End();
    }

}
