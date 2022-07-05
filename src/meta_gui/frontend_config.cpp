#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "frontends/frontend_catalogue.hpp"
#include "games/game_catalogue.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t running_few_idx = 0;
    uint32_t selected_few_idx = 0;

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
        // collect all frontend_wraps compatible with the current base game variant
        std::vector<Frontends::FrontendWrap*> compatible_few{};
        std::vector<uint32_t> compatible_few_idx{};
        Games::BaseGameVariant* bgv = Games::game_catalogue[base_game_idx].variants[game_variant_idx];
        bool selected_few_compatible = false;
        for (int i = 0; i < Frontends::frontend_catalogue.size(); i++) {
                if (Frontends::frontend_catalogue[i]->base_game_variant_compatible(bgv)) {
                    compatible_few.push_back(Frontends::frontend_catalogue[i]);
                    compatible_few_idx.push_back(i);
                    if (i == running_few_idx) {
                        selected_few_compatible = true;
                    }
                }
        }
        // if a frontend is running, which is not the empty frontend, and not compatible, unload it
        if (running_few_idx > 0 && !selected_few_compatible && selected_few_idx > 0) {
            selected_few_idx = 0;
            f_event_any es;
            f_event_create_type(&es, EVENT_TYPE_FRONTEND_UNLOAD);
            f_event_queue_push(&Control::main_client->inbox, &es);
        }
        // if the selected frontend is no longer compatible, but there are alternatives, select the first one of them instead
        if (compatible_few.size() > 0 && !selected_few_compatible) {
            selected_few_idx = compatible_few_idx[0];
        }
        bool fronend_running = (running_few_idx > 0);
        // draw game start,stop,restart
        // locks all pre loading input elements if frontend is running, stop is only available if running
        bool disable_fronend_loader = compatible_few.size() == 0;
        if (disable_fronend_loader) {
            ImGui::BeginDisabled();
        }
        if (fronend_running) {
            if (ImGui::Button("Restart")) {
                f_event_any es;
                f_event_create_frontend_load(&es, Frontends::frontend_catalogue[selected_few_idx]->new_frontend());
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
                f_event_create_frontend_load(&es, Frontends::frontend_catalogue[selected_few_idx]->new_frontend());
                f_event_queue_push(&Control::main_client->inbox, &es);
            }
        }
        if (disable_fronend_loader) {
            ImGui::EndDisabled();
        }
        // draw frontend_wrap combo box, show only compatible ones
        bool disable_frontend_selection = (compatible_few.size() < 2);
        if (compatible_few.size() == 0) {
            selected_few_idx = 0;
        }
        if (fronend_running || disable_frontend_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Frontend", Frontends::frontend_catalogue[selected_few_idx]->name, disable_frontend_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (int i = 0; i < compatible_few.size(); i++) {
                uint32_t frontend_idx = compatible_few_idx[i];
                bool is_selected = (selected_few_idx == frontend_idx);
                if (ImGui::Selectable(compatible_few[i]->name, is_selected)) {
                    selected_few_idx = frontend_idx;
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
            Frontends::frontend_catalogue[selected_few_idx]->draw_options();
            if (fronend_running) {
                ImGui::EndDisabled();
            }
        }
        ImGui::Separator();
        if (fronend_running) {
            if (ImGui::CollapsingHeader("Graphics Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                Control::main_client->frontend->draw_options();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::CollapsingHeader("Graphics Options", ImGuiTreeNodeFlags_Leaf);
            ImGui::EndDisabled();
        }
        ImGui::End();
    }

}
