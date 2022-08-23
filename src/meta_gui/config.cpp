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
            case CJ_TYPE_COL4U:
            case CJ_TYPE_COL4F:
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
        cfg_runlock(Control::main_client->dd.cfg_lock);
        
        ImGui::End();
    }

}
