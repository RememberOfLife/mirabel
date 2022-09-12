#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/config.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    void show_config_ovac(cj_ovac* ovac)
    {
        //TODO how to make values adjustable? imgui needs a value pointer, but there is no real guarantee the config object will event exist after dropping the lock for the frame
        //TODO can keep a second copy of the whole config tree, on render needs to run through all open nodes anyway, just update them on the fly, on value update true json tree is guaranteed to have the corresponding node available
        switch (ovac->type) {
            case CJ_TYPE_NONE: {
                assert(0);
            } break;
            case CJ_TYPE_OBJECT: {
                ImGui::Text("obj %u children", ovac->child_count);
                for (uint32_t i = 0; i < ovac->child_count; i++) {
                    if (ovac->children[i]->type == CJ_TYPE_OBJECT || ovac->children[i]->type == CJ_TYPE_ARRAY) {
                        if (ImGui::TreeNode(ovac->children[i]->label_str)) {
                            show_config_ovac(ovac->children[i]);
                            ImGui::TreePop();
                        }
                    } else {
                        show_config_ovac(ovac->children[i]);
                    }
                }
            } break;
            case CJ_TYPE_ARRAY: {
                ImGui::Text("arr %u children", ovac->child_count);
                char strid[16];
                for (uint32_t i = 0; i < ovac->child_count; i++) {
                    if (ovac->children[i]->type == CJ_TYPE_OBJECT || ovac->children[i]->type == CJ_TYPE_ARRAY) {
                        sprintf(strid, "[%u]", i);
                        if (ImGui::TreeNode(strid, "[%u]", i)) {
                            show_config_ovac(ovac->children[i]);
                            ImGui::TreePop();
                        }
                    } else {
                        show_config_ovac(ovac->children[i]);
                    }
                }
            } break;
            case CJ_TYPE_VNULL: {
                ImGui::TextDisabled("null");
            } break;
            case CJ_TYPE_U64: {
                ImGui::Text("u64: %lu", ovac->v.u64);
            } break;
            case CJ_TYPE_F32: {
                ImGui::Text("f32: %f", ovac->v.f32);
            } break;
            case CJ_TYPE_BOOL: {
                ImGui::Text("b: %s", ovac->v.b ? "true" : "false");
            } break;
            case CJ_TYPE_STRING: {
                ImGui::Text("str: %s", ovac->v.s.str);
            } break;
            case CJ_TYPE_COUNT:
                break;
        }
    }

    void config_registry_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Config Registry", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        //TODO multi tab? 1 for raw editing, other as styled configs?
        //TODO use imgui treenodes to repr json

        cfg_rlock(Control::main_client->dd.cfg_lock);
        show_config_ovac(Control::main_client->dd.cfg);
        /*{
            static bool init = false;
            if (!init) {
                init = true;
                cj_ovac* cfg = Control::main_client->dd.cfg;
                cj_object_append(cfg, "abc", cj_create_vnull());
                cj_object_append(cfg, "123", cj_create_f32(7.25));
                cj_ovac* a1 = cj_create_object(0);
                cj_object_append(a1, "a_key", cj_create_vnull());
                cj_object_append(a1, "anotherone", cj_create_str(200, "some string"));
                cj_object_append(cfg, "obj_key", a1);
                cj_ovac* a2 = cj_create_array(0);
                cj_array_append(a2, cj_create_u64(1));
                cj_array_append(a2, cj_create_u64(3));
                cj_array_append(a2, cj_create_u64(2));
                cj_array_append(a2, cj_create_u64(4));
                cj_object_append(cfg, "arr_key", a2);
                cj_object_append(cfg, "17", cj_create_u64(42));
                cj_object_append(cfg, "mybool", cj_create_bool(false));
            }
            static char* serializedstr = (char*)malloc(8000);
            cj_serialize(serializedstr, Control::main_client->dd.cfg, true);
            ImGui::Separator();
            ImGui::Text("measure: %zu", cj_measure(Control::main_client->dd.cfg, true));
            ImGui::Text("%s", serializedstr);
            ImGui::Text("real measure: %zu", strlen(serializedstr) + 1);
            cj_serialize(serializedstr, Control::main_client->dd.cfg, false);
            ImGui::Separator();
            ImGui::Text("measure: %zu", cj_measure(Control::main_client->dd.cfg, false));
            ImGui::Text("%s", serializedstr);
            ImGui::Text("real measure: %zu", strlen(serializedstr) + 1);
            ImGui::Separator();
            static char* readin = (char*)malloc(8000);
            static bool readininit = false;
            if (!readininit) {
                readininit = true;
                readin[0] = '\0';
            }
            ImGui::InputTextMultiline("json input", readin, 8000);
            if (ImGui::Button("parse")) {
                cj_ovac* newcfg = cj_deserialize(readin);
                if (newcfg) {
                    cj_ovac_destroy(Control::main_client->dd.cfg);
                    Control::main_client->dd.cfg = newcfg;
                }
            }
        }*/
        cfg_runlock(Control::main_client->dd.cfg_lock);
        
        ImGui::End();
    }

}
