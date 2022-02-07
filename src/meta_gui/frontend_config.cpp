#include <cstdint>

#include "imgui.h"

#include "frontends/frontend_catalogue.hpp"
#include "games/game_catalogue.hpp"
#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t frontend_wrap_idx = 0;

    void frontend_config_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(230, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(300, 80), ImGuiCond_FirstUseEver);
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
                    if (i == frontend_wrap_idx) {
                        selected_few_compatible = true;
                    }
                }
        }
        if (!selected_few_compatible) {
            frontend_wrap_idx = 0;
            StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_frontend_event(StateControl::EVENT_TYPE_FRONTEND_LOAD, Frontends::frontend_catalogue[frontend_wrap_idx]->new_frontend()));
        }
        // draw frontend_wrap combo box, show only compatible ones
        bool disable_frontend_selection = (compatible_few.size() == 1);
        if (disable_frontend_selection) {
            ImGui::BeginDisabled();
        }
        if (ImGui::BeginCombo("Frontend", Frontends::frontend_catalogue[frontend_wrap_idx]->name, disable_frontend_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
            for (int i = 0; i < compatible_few.size(); i++) {
                uint32_t frontend_idx = compatible_few_idx[i];
                bool is_selected = (frontend_wrap_idx == frontend_idx);
                if (ImGui::Selectable(compatible_few[i]->name, is_selected)) {
                    frontend_wrap_idx = frontend_idx;
                    StateControl::main_ctrl->t_gui.inbox.push(StateControl::event::create_frontend_event(StateControl::EVENT_TYPE_FRONTEND_LOAD, Frontends::frontend_catalogue[frontend_wrap_idx]->new_frontend()));
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (disable_frontend_selection) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        StateControl::main_ctrl->t_gui.frontend->draw_options();
        ImGui::End();
    }

}
