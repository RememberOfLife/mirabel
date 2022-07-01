#include <cstdint>

#include "imgui.h"
#include "surena/engine.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "engines/engine_catalogue.hpp"
#include "engines/engine_manager.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t engine_idx = 0;

    void engine_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 540), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Engine", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton;
        if (ImGui::BeginTabBar("engines", tab_bar_flags))
        {
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                Control::main_client->engine_mgr->add_container(PLAYER_NONE);
            }

            for (int te_idx = 0; te_idx < Control::main_client->engine_mgr->engines.size(); te_idx++) {
                Engines::EngineManager::engine_container& tec = *Control::main_client->engine_mgr->engines[te_idx]; // the engine container

                //TODO change active tab color, hardly distinguishable otherwise
                //TODO focus new tabs?
                bool* p_open = &tec.open;
                //TODO maybe change tab colors like in the log window to display what engines are currently started/searching etc..
                ImGuiTabItemFlags item_flags = ImGuiTabItemFlags_None;
                if (tec.ai_slot > PLAYER_NONE) {
                    item_flags |= ImGuiTabItemFlags_UnsavedDocument;
                    p_open = NULL;
                }
                if (((p_open && *p_open == true) || p_open == NULL) && ImGui::BeginTabItem(tec.name, p_open, item_flags))
                {
                    // everything below here should be from an engine container indexed by the active engine tab, displayed inside the tab bar


                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::Button("X")) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::InputText("##", tec.name_swap, Engines::EngineManager::STR_BUF_MAX)) {
                            tec.swap_names = true;
                        }
                        ImGui::EndPopup();
                    } else if (tec.swap_names) {
                        Control::main_client->engine_mgr->rename_container(te_idx);
                    }
                    
                    bool engine_running = (tec.eq != NULL);

                    bool disable_startstop = tec.stopping;
                    if (disable_startstop) {
                        ImGui::BeginDisabled();
                    }
                    if (engine_running) {
                        if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                            Control::main_client->engine_mgr->stop_container(te_idx);
                        }
                    } else {
                        if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                            Control::main_client->engine_mgr->start_container(te_idx);
                        }
                    }
                    if (disable_startstop) {
                        ImGui::EndDisabled();
                    }

                    bool disable_engine_selection = Engines::engine_catalogue.size() < 2;
                    if (engine_running || disable_engine_selection) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::BeginCombo("Engine", Engines::engine_catalogue[tec.catalogue_idx]->engine_name, disable_engine_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
                        for (int i = 0; i < Engines::engine_catalogue.size(); i++) {
                            bool is_selected = (tec.catalogue_idx == i);
                            if (ImGui::Selectable(Engines::engine_catalogue[i]->engine_name, is_selected)) {
                                tec.catalogue_idx = i;
                                //TODO destruct old load data, construct new load data with new engine loader
                            }
                            // set the initial focus when opening the combo
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    if (engine_running || disable_engine_selection) {
                        ImGui::EndDisabled();
                    }

                    ImGui::Separator();

                    ImGui::TextDisabled("<options currently disabled>"); //TODO reenable options when wrapper exists
                    if (false && ImGui::CollapsingHeader(engine_running ? "Load Options [locked]" : "Load Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (engine_running) {
                            ImGui::BeginDisabled();
                        }
                        
                        ImGui::TextDisabled("<no options>"); //TODO from engine wrapper in engine catalogue

                        if (engine_running) {
                            ImGui::EndDisabled();
                        }
                    }

                    ImGui::Separator();

                    if (engine_running) {

                        const ImVec2 cell_padding(8.0f, 0.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
                        if (ImGui::BeginTable("idtable", 2))
                        {
                            ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted("ID Name");
                            ImGui::TableSetColumnIndex(1);
                            if (tec.id_name != NULL) {
                                ImGui::TextUnformatted(tec.id_name);
                            } else {
                                ImGui::TextDisabled("<unavailable>");
                            }

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted("ID Author");
                            ImGui::TableSetColumnIndex(1);
                            if (tec.id_author != NULL) {
                                ImGui::TextUnformatted(tec.id_author);
                            } else {
                                ImGui::TextDisabled("<unavailable>");
                            }

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted("HB Pending / LR");
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Heartbeat Pending / Last Response");
                            }
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%u / %.1fs", tec.heartbeat_next_id - tec.heartbeat_last_response - 1, (float)(SDL_GetTicks64() - tec.heartbeat_last_ticks) / 1000);

                            ImGui::EndTable();
                        }
                        ImGui::PopStyleVar();

                        bool engine_searching = tec.searching;

                        ImGui::Separator();

                        // only sends slider/text change when the user releases the control, otherwise incurs very many events to the engine
                        //TODO offer similar button for reset to default value?
                        if (ImGui::CollapsingHeader(engine_searching ? "Runtime Options [locked]" : "Runtime Options")) {

                            if (engine_searching) {
                                ImGui::BeginDisabled();
                            }

                            for (int opt_idx = 0; opt_idx < tec.options.size(); opt_idx++) {
                                ee_engine_option& eopt = tec.options[opt_idx];
                                bool eopt_changed = false;
                                switch (eopt.type) {
                                    case EE_OPTION_TYPE_CHECK: {
                                        if (ImGui::Checkbox(eopt.name, &eopt.value.check)) {
                                            tec.submit_option(&eopt);
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_SPIN: {
                                        if (ImGui::SliderScalar(eopt.name, ImGuiDataType_U64, &eopt.value.spin, &eopt.l.mm.min, &eopt.l.mm.max, NULL, ImGuiSliderFlags_AlwaysClamp)) {
                                            eopt_changed = true;
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("min: %lu\nmax: %lu", eopt.l.mm.min, eopt.l.mm.max);
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_COMBO: {
                                        //TODO unstatic
                                        static int combo_selected = 0; //TODO use value to set this
                                        if (ImGui::Combo(eopt.name, &combo_selected, eopt.l.v.var)) {
                                            MetaGui::logf("engine option \"%s\": changed\n", eopt.name);
                                            //TODO
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_BUTTON: {
                                        if (ImGui::Button(eopt.name)) {
                                            tec.submit_option(&eopt);
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_STRING: {
                                        if (ImGui::InputText(eopt.name, eopt.value.str, Engines::EngineManager::STR_BUF_MAX)) {
                                            eopt_changed = true;
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_SPIND: {
                                        if (ImGui::SliderScalar(eopt.name, ImGuiDataType_Double, &eopt.value.spind, &eopt.l.mmd.min, &eopt.l.mmd.max, NULL, ImGuiSliderFlags_AlwaysClamp)) {
                                            eopt_changed = true;
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("min: %f\nmax: %f", eopt.l.mmd.min, eopt.l.mmd.max);
                                        }
                                    } break;
                                    case EE_OPTION_TYPE_U64: {
                                        if (ImGui::InputScalar(eopt.name, ImGuiDataType_U64, &eopt.value.u64)) {
                                            if (eopt.value.u64 > eopt.l.mm.max) {
                                                eopt.value.u64 = eopt.l.mm.max;
                                            }
                                            if (eopt.value.u64 < eopt.l.mm.min) {
                                                eopt.value.u64 = eopt.l.mm.min;
                                            }
                                            eopt_changed = true;
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("min: %lu\nmax: %lu", eopt.l.mm.min, eopt.l.mm.max);
                                        }
                                    } break;
                                    default: {

                                    } break;
                                }
                                if (eopt_changed) {
                                    tec.options_changed[opt_idx] = eopt_changed;
                                }
                                if (tec.options_changed[opt_idx]) {
                                    ImGui::SameLine();

                                    float button_width = ImGui::CalcTextSize("S").x + ImGui::GetStyle().FramePadding.x * 2.f;
                                    float width_needed = /*ImGui::GetStyle().ItemSpacing.x + */button_width;
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width_needed);

                                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
                                    if (ImGui::Button("S")) {
                                        // SYNCED back to engine
                                        tec.options_changed[opt_idx] = false;
                                        tec.submit_option(&eopt);
                                    }
                                    ImGui::PopStyleColor(3);
                                }
                            }

                            if (tec.options.size() == 0) {
                                ImGui::TextDisabled("<no options>");
                            }

                            if (engine_searching) {
                                ImGui::EndDisabled();
                            }

                        }

                        ImGui::Separator();

                        //TODO some way to visually highlight the start/stop button
                        if (ImGui::ArrowButton("##search_constraints_btn", tec.search_constraints_open ? ImGuiDir_Down : ImGuiDir_Right)) {
                            tec.search_constraints_open = !tec.search_constraints_open;
                        }
                        ImGui::SameLine();
                        if (engine_searching) {
                            if (tec.e.methods->features.running_bestmove) {
                                if (ImGui::Button("POLL BM")) {
                                    tec.search_poll_bestmove();
                                }
                                ImGui::SameLine();
                            }
                            if (ImGui::Button("STOP SEARCH", ImVec2(-1.0f, 0.0f))) {
                                tec.stop_search();
                            }
                        } else {
                            if (ImGui::Button("START SEARCH", ImVec2(-1.0f, 0.0f))) {
                                tec.start_search();
                            }
                        }
                        
                        if (tec.search_constraints_open) {
                            if (engine_searching) {
                                ImGui::BeginDisabled();
                            }
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
                            if (ImGui::BeginTable("table_search_constraints", 1, ImGuiTableFlags_Borders))
                            {
                                //TODO combo box for which player to search? (show name in parens)

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::InputScalar("timeout (ms)", ImGuiDataType_U32, &tec.search_constraints.timeout);

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Checkbox("ponder", &tec.search_constraints.ponder);

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Checkbox("use timectl", &tec.search_constraints_timectl);

                                ImGui::EndTable();
                            }
                            ImGui::PopStyleVar();
                            if (engine_searching) {
                                ImGui::EndDisabled();
                            }
                        }

                        ImGui::Separator();

                        if (ImGui::CollapsingHeader("Search Info")) {
                            if (engine_searching) {
                                ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.16, 0.92, 0.53, 1));
                            } //TODO want more colors, ? e.g. maybe show stale info in the table? or just yellow if not running but info still there
                            ImGuiTableFlags searchinfo_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg; // replace Border with ImGuiTableFlags_BordersOuter?
                            if (ImGui::BeginTable("searchinfo_table", 2, searchinfo_table_flags))
                            {
                                // ImGui::TableSetupColumn("One");
                                // ImGui::TableSetupColumn("Two");
                                // ImGui::TableHeadersRow();

                                //TODO might be able to use a subtle progressbar to hint relative amounts, e.g. hashfull, time used from timeout, etc..
                                
                                //TODO right align these values

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("time");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_TIME) {
                                    ImGui::Text("%u", tec.searchinfo.time);
                                } else {
                                    ImGui::TextDisabled("---");
                                }
                            
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("depth");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_DEPTH) {
                                    ImGui::Text("%u", tec.searchinfo.depth);
                                } else {
                                    ImGui::TextDisabled("---");
                                }

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("seldepth");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_SELDEPTH) {
                                    ImGui::Text("%u", tec.searchinfo.seldepth);
                                } else {
                                    ImGui::TextDisabled("---");
                                }
                            
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("nodes");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NODES) {
                                    ImGui::Text("%lu", tec.searchinfo.nodes);
                                } else {
                                    ImGui::TextDisabled("---");
                                }
                            
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("nps");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NPS) {
                                    ImGui::Text("%lu", tec.searchinfo.nps);
                                } else {
                                    ImGui::TextDisabled("---");
                                }
                            
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("hashfull");
                                ImGui::TableSetColumnIndex(1);
                                if (tec.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_HASHFULL) {
                                    ImGui::Text("%f", tec.searchinfo.hashfull);
                                } else {
                                    ImGui::TextDisabled("---");
                                }

                                ImGui::EndTable();
                            }
                            if (engine_searching) {
                                ImGui::PopStyleColor();
                            }
                        }

                        ImGui::Separator();

                        ImGui::TextUnformatted("Best move per player:");
                        if (tec.bestmove.count > 0) {
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
                            if (ImGui::BeginTable("bestmoves", 3, ImGuiTableFlags_Borders))
                            {
                                ImGui::TableSetupColumn("player");
                                ImGui::TableSetupColumn("move");
                                ImGui::TableSetupColumn("conf");
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < tec.bestmove.count; i++) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("%03hhu", tec.bestmove.player[i]);
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%s", tec.bestmove_strings[i]);
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::Text("%.3f", tec.bestmove.confidence[i]);
                                    //TODO eval
                                }

                                ImGui::EndTable();
                            }
                            ImGui::PopStyleVar();
                        } else {
                            ImGui::SameLine();
                            ImGui::TextDisabled("<unavailable>");
                        }

                    }

                    ImGui::EndTabItem();
                }

                if (*p_open == false) {
                    Control::main_client->engine_mgr->remove_container(te_idx);
                }

            }

            if (Control::main_client->engine_mgr->engines.size() == 0) {
                ImGui::TextDisabled("<no open engines>");
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

}
