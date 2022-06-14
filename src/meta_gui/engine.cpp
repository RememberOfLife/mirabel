#include <cstdint>

#include "imgui.h"
#include "surena/engine.h"

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
        ImGui::SetNextWindowSize(ImVec2(300, 540), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Engine", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        //TODO use tabs here to fill the available engine slots

        static std::vector<int> active_tabs = {0, 1, 2, 3};
        static int next_tab_id = 0;

        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton;

        if (ImGui::BeginTabBar("engines", tab_bar_flags))
        {
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                active_tabs.push_back(next_tab_id++);
            }

            for (int n = 0; n < active_tabs.size(); )
            {
                bool open = true;
                char name[16];
                snprintf(name, IM_ARRAYSIZE(name), "%04d", active_tabs[n]);
                //TODO maybe change tab colors like in the log window to display what engines are currently started/searching etc..
                //TODO ImGuiTabItemFlags_UnsavedDocument tab item flag for PLAYER controlling engines
                if (ImGui::BeginTabItem(name, &open, ImGuiTabItemFlags_None))
                {
                    //TODO display the engine details page
                    ImGui::EndTabItem();
                }

                if (!open)
                    active_tabs.erase(active_tabs.begin() + n);
                else
                    n++;
            }

            ImGui::EndTabBar();
        }

        // everything below here should be from an engine container indexed by the active engine tab, displayed inside the tab bar

        static bool engine_running = false;

        if (engine_running) {
            if (ImGui::Button("Restart")) {
                
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(-1.0f, 0.0f))) {
                engine_running = false;
            }
        } else {
            if (ImGui::Button("Start", ImVec2(-1.0f, 0.0f))) {
                engine_running = true;
            }
        }

        if (engine_running) {
            ImGui::BeginDisabled();
        }
        static int current_combo = 0;
        if (ImGui::Combo("Engine", &current_combo, "Random\0The Best\0")) {

        }
        if (engine_running) {
            ImGui::EndDisabled();
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader(engine_running ? "Load Options [locked]" : "Load Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (engine_running) {
                ImGui::BeginDisabled();
            }
            
            ImGui::TextDisabled("<no options>"); //TODO from engine wrapper

            if (engine_running) {
                ImGui::EndDisabled();
            }
        }

        ImGui::Separator();

        if (engine_running) {

            static ImVec2 cell_padding(8.0f, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
            if (ImGui::BeginTable("table1", 2))
            {
                ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("ID Name");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted("RandomEngine");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("ID Author");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted("RNGesus");

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            ImGui::Separator();

            static bool engine_searching = false;

            static bool search_constraints_open = false; //TODO finda better place for the search constraints
            if (ImGui::ArrowButton("##search_constraints_btn", search_constraints_open ? ImGuiDir_Down : ImGuiDir_Right)) {
                search_constraints_open = !search_constraints_open;
            }
            ImGui::SameLine();
            if (ImGui::Button(engine_searching ? "STOP SEARCH" : "START SEARCH", ImVec2(-1.0f, 0.0f))) {
                engine_searching = !engine_searching;
            }
            if (search_constraints_open) {
                
                if (engine_searching) {
                    ImGui::BeginDisabled();
                }

                static uint32_t timeout = 0;

                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
                if (ImGui::BeginTable("table_search_constraints", 1, ImGuiTableFlags_Borders))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::InputScalar("timeout (ms)", ImGuiDataType_U32, &timeout);

                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();

                if (engine_searching) {
                    ImGui::EndDisabled();
                }

            }

            ImGui::Separator();

            ImGui::Text("BESTMOVE (p%hhu): %s", (uint8_t)2, "5d3<221*");

            static ee_engine_searchinfo esinfo = {
                .flags = EE_SEARCHINFO_FLAG_TYPE_SCORE | EE_SEARCHINFO_FLAG_TYPE_NODES,
                .score_player = 1,
                .score_eval = 0.5,
                .nodes = 50000,
            };

            if (engine_searching) {
                ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.16, 0.92, 0.53, 1));
            } //TODO want more colors, ? e.g. maybe show stale info in the table? or just yellow if not running but info still there
            static ImGuiTableFlags searchinfo_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg; // replace Border with ImGuiTableFlags_BordersOuter?
            if (ImGui::BeginTable("searchinfo_table", 2, searchinfo_table_flags))
            {
                // ImGui::TableSetupColumn("One");
                // ImGui::TableSetupColumn("Two");
                // ImGui::TableHeadersRow();
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("time");
                ImGui::TableSetColumnIndex(1);
                if (esinfo.flags & EE_SEARCHINFO_FLAG_TYPE_TIME) {
                    ImGui::Text("%u", esinfo.time);
                } else {
                    ImGui::TextDisabled("---");
                }
            
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("depth");
                ImGui::TableSetColumnIndex(1);
                if (esinfo.flags & EE_SEARCHINFO_FLAG_TYPE_DEPTH) {
                    ImGui::Text("%u", esinfo.depth);
                } else {
                    ImGui::TextDisabled("---");
                }
            
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("nps");
                ImGui::TableSetColumnIndex(1);
                if (esinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NPS) {
                    ImGui::Text("%lu", esinfo.nps);
                } else {
                    ImGui::TextDisabled("---");
                }
            
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("nodes");
                ImGui::TableSetColumnIndex(1);
                if (esinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NODES) {
                    ImGui::Text("%lu", esinfo.nodes);
                } else {
                    ImGui::TextDisabled("---");
                }
            
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("hashfull");
                ImGui::TableSetColumnIndex(1);
                if (esinfo.flags & EE_SEARCHINFO_FLAG_TYPE_HASHFULL) {
                    ImGui::Text("%f", esinfo.hashfull);
                } else {
                    ImGui::TextDisabled("---");
                }

                // player_id score_player;
                // float score_eval;
                // uint32_t pc_c; // count of pv nodes
                // player_id* pv_p;
                // move_code* pv_m;
                // char* str; //TODO these just get logged directly to the engines log tab

                ImGui::EndTable();
            }
            if (engine_searching) {
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            static char combo_val[100] = "abc\0";
            static char combo_var[100] = "abc\0def\0\0";
            static char str_buf[100] = "str default\0";
            static ee_engine_option eopts[5] = {
                ee_engine_option{
                    .name = strdup("option 1"),
                    .type = EE_OPTION_TYPE_CHECK,
                    .value = {
                        .check = false,
                    },
                },
                ee_engine_option{
                    .name = strdup("mybtn"),
                    .type = EE_OPTION_TYPE_BUTTON,
                    .value = {
                        .check = false,
                    },
                },
                ee_engine_option{
                    .name = strdup("slider"),
                    .type = EE_OPTION_TYPE_SPIN,
                    .value = {
                        .spin = 17,
                    },
                    .mm = {
                        .min = 10,
                        .max = 20,
                    },
                },
                ee_engine_option{
                    .name = strdup("mycombo"),
                    .type = EE_OPTION_TYPE_COMBO,
                    .value = {
                        .combo = combo_val,
                    },
                    .v = {
                        .var = combo_var,
                    },
                },
                ee_engine_option{
                    .name = strdup("thestr"),
                    .type = EE_OPTION_TYPE_STRING,
                    .value = {
                        .str = str_buf,
                    },
                },
            };

            //TODO only send slider/text change when the user releases the control, otherwise incurs very many events to the engine
            static bool eopts_changed[5] = {
                false,
                false,
                false,
                false,
                false,
            };
            //TODO offer similar button for reset to default value?

            if (ImGui::CollapsingHeader(engine_searching ? "Runtime Options [locked]" : "Runtime Options", ImGuiTreeNodeFlags_DefaultOpen)) {

                if (engine_searching) {
                    ImGui::BeginDisabled();
                }

                for (int opt_idx = 0; opt_idx < 5; opt_idx++) {
                    ee_engine_option& eopt = eopts[opt_idx];
                    switch (eopt.type) {
                        case EE_OPTION_TYPE_CHECK: {
                            if (ImGui::Checkbox(eopt.name, &eopt.value.check)) {
                                MetaGui::logf("engine option \"%s\": changed\n", eopt.name);
                            }
                        } break;
                        case EE_OPTION_TYPE_SPIN: {
                            if (ImGui::SliderScalar(eopt.name, ImGuiDataType_U64, &eopt.value.spin, &eopt.mm.min, &eopt.mm.max, NULL, ImGuiSliderFlags_AlwaysClamp)) {
                                eopts_changed[opt_idx] = true;
                            }
                        } break;
                        case EE_OPTION_TYPE_COMBO: {
                            static int combo_selected = 0; //TODO use value to set this
                            if (ImGui::Combo(eopt.name, &combo_selected, eopt.v.var)) {
                                MetaGui::logf("engine option \"%s\": changed\n", eopt.name);
                            }
                        } break;
                        case EE_OPTION_TYPE_BUTTON: {
                            if (ImGui::Button(eopt.name)) {
                                MetaGui::logf("engine option \"%s\": changed\n", eopt.name);
                            }
                        } break;
                        case EE_OPTION_TYPE_STRING: {
                            if (ImGui::InputText(eopt.name, eopt.value.str, 200)) {
                                eopts_changed[opt_idx] = true;
                            }
                        } break;
                        case EE_OPTION_TYPE_SPIND: {
                            if (ImGui::SliderScalar(eopt.name, ImGuiDataType_Double, &eopt.value.spind, &eopt.mmd.min, &eopt.mmd.max, NULL, ImGuiSliderFlags_AlwaysClamp)) {
                                eopts_changed[opt_idx] = true;
                            }
                        } break;
                        default: {

                        } break;
                    }
                    if (eopts_changed[opt_idx]) {
                        ImGui::SameLine();

                        float button_width = ImGui::CalcTextSize("S").x + ImGui::GetStyle().FramePadding.x * 2.f;
                        float width_needed = /*ImGui::GetStyle().ItemSpacing.x + */button_width;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - width_needed);

                        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
                        if (ImGui::Button("S")) {
                            // SYNCED back to engine
                            eopts_changed[opt_idx] = false;
                            MetaGui::logf("engine option \"%s\": changed\n", eopt.name);
                        }
                        ImGui::PopStyleColor(3);
                    }
                }

                if (engine_searching) {
                    ImGui::EndDisabled();
                }

            }

        }

        ImGui::TextColored(ImVec4{0.87, 0.17, 0.17, 1}, "<mockup>");
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
