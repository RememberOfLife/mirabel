#include <cstdint>

#include "imgui.h"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t base_game_idx = 0;
    uint32_t game_variant_idx = 0;

    void game_config_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(50, 80), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Game Config", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }
        bool game_running = (StateControl::main_ctrl->t_gui.game != NULL);
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
        // draw options panel
        Games::game_catalogue[base_game_idx].variants[game_variant_idx]->draw_options();
        if (game_running) {
            ImGui::EndDisabled();
        }
        // draw game start,stop,restart
        // locks all input elements above if game is running, stop is only available if running
        ImGui::Separator();
        if (game_running) {
            if (ImGui::Button("Restart")) {
                StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_GAME_UNLOAD));
                StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_game_event(StateControl::EVENT_TYPE_GAME_LOAD, Games::game_catalogue[base_game_idx].variants[game_variant_idx]->new_game()));
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                StateControl::main_ctrl->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_GAME_UNLOAD));
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_game_event(StateControl::EVENT_TYPE_GAME_LOAD, Games::game_catalogue[base_game_idx].variants[game_variant_idx]->new_game()));
            }
        }
        ImGui::Separator();
        if (!game_running) {
            ImGui::End();
            return;
        }
        // draw internal state editor, only if a game is running
        Games::game_catalogue[base_game_idx].variants[game_variant_idx]->draw_state_editor();
        ImGui::End();
    }

}
