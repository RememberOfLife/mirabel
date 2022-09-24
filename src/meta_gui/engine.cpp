#include <cstdint>

#include "imgui.h"
#include "surena/engine.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/client.hpp"
#include "control/plugins.hpp"
#include "engines/engine_manager.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    uint32_t engine_idx = 0;

    void engine_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 540), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Engine", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        Control::PluginManager& plugin_mgr = Control::main_client->plugin_mgr;

        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton;
        if (ImGui::BeginTabBar("engines", tab_bar_flags)) {
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
                if (((p_open && *p_open == true) || p_open == NULL) && ImGui::BeginTabItem(tec.name, p_open, item_flags)) {
                    // everything below here should be from an engine container indexed by the active engine tab, displayed inside the tab bar

                    const char* selected_name = "<NONE>";
                    if (tec.catalogue_idx > 0) {
                        if (plugin_mgr.engine_lookup.find(tec.catalogue_idx) == plugin_mgr.engine_lookup.end()) {
                            // catalogue idx has been invalidated
                            tec.catalogue_idx = 0;
                        } else {
                            selected_name = plugin_mgr.engine_lookup[tec.catalogue_idx]->get_name();
                        }
                    }

                    if (ImGui::BeginPopupContextItem()) {
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

                    bool disable_startstop = tec.stopping || (tec.catalogue_idx == 0);
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

                    //BUG if a method is removed while its options were still on display there is a memory leak because no one can clean it up ==> NEED used refc in plugin impls
                    bool disable_engine_selection = plugin_mgr.engine_catalogue.size() == 0;
                    // catalogue idx should already be 0 if no engines exists in the catalogue
                    if (engine_running || disable_engine_selection) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::BeginCombo("Engine", selected_name, disable_engine_selection ? ImGuiComboFlags_NoArrowButton : ImGuiComboFlags_None)) {
                        // offer idx 0 element, so selection can be reset, and the last engine can be unloaded from plugins
                        bool is_selected = (tec.catalogue_idx == 0);
                        if (ImGui::Selectable("<NONE>", is_selected)) {
                            // destruct old load data
                            if (tec.catalogue_idx > 0) {
                                plugin_mgr.engine_lookup[tec.catalogue_idx]->destroy_opts(tec.load_options);
                                tec.load_options = NULL;
                            }
                            tec.catalogue_idx = 0;
                        }
                        // set the initial focus when opening the combo
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        for (Control::EngineImpl it : plugin_mgr.engine_catalogue) {
                            is_selected = (tec.catalogue_idx == it.map_idx);
                            if (ImGui::Selectable(it.get_name(), is_selected)) {
                                // destruct old load data, construct new load data with new engine loader
                                if (tec.catalogue_idx > 0) {
                                    plugin_mgr.engine_lookup[tec.catalogue_idx]->destroy_opts(tec.load_options);
                                    tec.load_options = NULL;
                                }
                                tec.catalogue_idx = it.map_idx;
                                if (tec.catalogue_idx > 0) {
                                    plugin_mgr.engine_lookup[tec.catalogue_idx]->create_opts(&tec.load_options);
                                }
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

                    if (tec.catalogue_idx > 0 && ImGui::CollapsingHeader(engine_running ? "Load Options [locked]" : "Load Options", ImGuiTreeNodeFlags_DefaultOpen)) {

                        ImGui::Separator();

                        if (engine_running) {
                            ImGui::BeginDisabled();
                        }

                        plugin_mgr.engine_lookup[tec.catalogue_idx]->display_opts(tec.load_options);

                        if (engine_running) {
                            ImGui::EndDisabled();
                        }
                    }

                    ImGui::Separator();

                    if (engine_running) {

                        const ImVec2 cell_padding(8.0f, 0.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
                        if (ImGui::BeginTable("idtable", 2)) {
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
                                        // the options combo points to the selected element in the v.var
                                        if (ImGui::BeginCombo(eopt.name, eopt.value.combo)) {
                                            char* p_sel_combo = eopt.l.v.var;
                                            while (*p_sel_combo != '\0') {
                                                bool is_selected = (strcmp(eopt.value.combo, p_sel_combo) == 0);
                                                if (ImGui::Selectable(p_sel_combo, is_selected)) {
                                                    eopt.value.combo = p_sel_combo;
                                                    tec.submit_option(&eopt); // this resubmits options even if the selected value didnt change! //TODO wanted behaviour?
                                                }
                                                // set the initial focus when opening the combo
                                                if (is_selected) {
                                                    ImGui::SetItemDefaultFocus();
                                                }
                                                p_sel_combo += strlen(p_sel_combo) + 1;
                                            }
                                            ImGui::EndCombo();
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
                                    float width_needed = /*ImGui::GetStyle().ItemSpacing.x + */ button_width;
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
                            if (ImGui::BeginTable("table_search_constraints", 1, ImGuiTableFlags_Borders)) {
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
                            if (ImGui::BeginTable("searchinfo_table", 2, searchinfo_table_flags)) {
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
                            if (ImGui::BeginTable("bestmoves", 3, ImGuiTableFlags_Borders)) {
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

} // namespace MetaGui
