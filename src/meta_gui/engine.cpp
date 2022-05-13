#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
// #include "engines/engine_catalogue.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t engine_idx = 0;

    void engine_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Engine", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        ImGui::TextColored(ImVec4{0.87, 0.17, 0.17, 1}, "<disabled>");
        ImGui::End();

        /*
        bool engine_running = Control::main_client->engine != NULL;
        // collect all engines compatible with the current base game variant
        std::vector<Engines::Engine*> compatible_engines{};
        std::vector<uint32_t> compatible_engines_idx{};
        Games::BaseGameVariant* bgv = Games::game_catalogue[base_game_idx].variants[game_variant_idx];
        bool selected_engine_compatible = false;
        for (int i = 0; i < Engines::engine_catalogue.size(); i++) {
                if (Engines::engine_catalogue[i]->base_game_variant_compatible(bgv)) {
                    compatible_engines.push_back(Engines::engine_catalogue[i]);
                    compatible_engines_idx.push_back(i);
                    if (i == engine_idx) {
                        selected_engine_compatible = true;
                    }
                }
        }
        if (!selected_engine_compatible) {
            engine_idx = 0;
            Control::main_client->inbox.push(Control::event(Control::EVENT_TYPE_ENGINE_UNLOAD));
        }
        // draw engine restart/start/stop
        if (engine_running) {
            if (ImGui::Button("Restart")) {
                Control::main_client->inbox.push(Control::event::create_engine_event(Control::EVENT_TYPE_ENGINE_LOAD, Engines::engine_catalogue[engine_idx]->new_engine()));
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                Control::main_client->inbox.push(Control::event(Control::EVENT_TYPE_ENGINE_UNLOAD));
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                Control::main_client->inbox.push(Control::event::create_engine_event(Control::EVENT_TYPE_ENGINE_LOAD, Engines::engine_catalogue[engine_idx]->new_engine()));
            }
        }
        // draw engine combo box, show only compatible ones
        bool disable_engine_selection = (compatible_engines.size() == 1);
        if (disable_engine_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Engine", Engines::engine_catalogue[engine_idx]->name, disable_engine_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (int i = 0; i < compatible_engines.size(); i++) {
                uint32_t i_engine_idx = compatible_engines_idx[i];
                bool is_selected = (engine_idx == i_engine_idx);
                if (ImGui::Selectable(compatible_engines[i]->name, is_selected)) {
                    engine_idx = i_engine_idx;
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (disable_engine_selection) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        if (engine_running) {
            ImGui::BeginDisabled();
        }
        Engines::engine_catalogue[engine_idx]->draw_loader_options();
        if (engine_running) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        if (engine_running) {
            Engines::engine_catalogue[engine_idx]->draw_state_options(Control::main_client->engine);
        }
        ImGui::End();
        */
    }

}
