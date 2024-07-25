#include <cstdint>
#include <cstdio>

#include <GL/glew.h>
#include "imgui.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_workspace_window(uint32_t workspace_id) //TODO workspace as arg
{
    bool opened = true;
    //TODO separate id from window title so we can change the window title without messing up imgui internal ids
    char window_title[128];
    sprintf(window_title, "Workspace###%u", workspace_id);
    ImVec2 mcenter = ImGui::GetMainViewport()->GetCenter(); //TODO better initial placement as center docked..
    ImGui::SetNextWindowPos(mcenter, ImGuiCond_FirstUseEver, ImVec2(0.5, 0.5));
    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(window_title, &opened)) {

        ImGuiTabBarFlags tabbar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable;
        if (ImGui::BeginTabBar("Breadcrumbs", tabbar_flags)) {
            if (ImGui::BeginTabItem("|")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("> Server")) {

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
                ImVec2 parent_pos = ImGui::GetCursorScreenPos();
                ImVec2 insize = ImGui::GetContentRegionAvail();
                ImGui::SetNextWindowPos(ImVec2(parent_pos.x + insize.x / 2, parent_pos.y), ImGuiCond_None, ImVec2(0.5, 0));
                const float window_size_min = 450;
                float window_size = insize.x;
                if (window_size > window_size_min) {
                    window_size = window_size_min;
                }
                ImGui::BeginChild("Server", ImVec2(window_size, 0), ImGuiChildFlags_None, window_flags);
                {
                    bool offline_server_available = true;
                    if (!offline_server_available) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Offline Server", ImVec2(-1, 2 * ImGui::GetTextLineHeightWithSpacing()))) {
                    }
                    if (!offline_server_available) {
                        ImGui::EndDisabled();
                    }
                    ImGui::Separator();
                    ImGui::Text("Saved Servers");
                    ImGui::SameLine();
                    ImGui::SmallButton("Clear"); //TODO right align
                    if (ImGui::BeginListBox("##Server List", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                        static int item_current_idx = 0;
                        for (int n = 0; n < 3; n++) {
                            const bool is_selected = (item_current_idx == n);
                            char selectable_server_name[64];
                            sprintf(selectable_server_name, "Server %i", n);
                            if (ImGui::Selectable(selectable_server_name, is_selected)) {
                                item_current_idx = n;
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::Separator();
                    ImGui::Text("Connection");
                    static char server_addr_buf[128] = "run.mirabel.dev";
                    uint16_t server_port = 61801;
                    ImGui::InputText("address", server_addr_buf, sizeof(server_addr_buf)); //TODO filter letters
                    ImGui::InputScalar("port", ImGuiDataType_U16, &server_port);
                    static bool server_saved = true;
                    ImGui::Checkbox("Save Server", &server_saved);
                    if (server_saved) {
                        static char server_short_name[32] = "";
                        ImGui::InputTextWithHint("saved name", server_addr_buf, server_short_name, sizeof(server_short_name));
                    }
                    ImGui::Button("Connect", ImVec2(-1, 0));
                    ImGui::Separator();
                    ImGui::Text("Status:");
                    ImGui::SameLine();
                    ImGui::Text("---");
                    ImGui::Button("PING", ImVec2(-1, 0));
                    if (ImGui::CollapsingHeader("Thumbprint: 01:23:45:67:89:AB:CD:FE")) {
                        //TODO push monospace imgui font here
                        for (int i = 0; i < 8; i++) {
                            for (int j = 0; j < 8; j++) {
                                ImGui::Text("%02x", i * j);
                                if (j < 8 - 1) {
                                    ImGui::SameLine();
                                }
                            }
                        }
                    }

                    //TODO verifail
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("> User")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("> Lobby")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("> Frontend")) {

                /////////

                ImVec2 content_size = ImGui::GetContentRegionAvail();
                float new_fbw = content_size.x;
                float new_fbh = content_size.y;
                bool skip_draw = false;
                if (new_fbw <= 0 || new_fbh <= 0) {
                    skip_draw = true; // dont draw if invisible, otherwise FBO errors
                }
                if (!skip_draw) {
                    if (new_fbw != fedd.fbw || new_fbh != fedd.fbh) {
                        // resized, change them
                        glBindFramebuffer(GL_FRAMEBUFFER, fedd.frontend_fbo);
                        glBindTexture(GL_TEXTURE_2D, fedd.frontend_tex);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_fbw, new_fbh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fedd.frontend_tex, 0);
                        glBindRenderbuffer(GL_RENDERBUFFER, fedd.frontend_rbo);
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fedd.fbw, fedd.fbh);
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fedd.frontend_rbo);
                        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                            mirabel_slogf(LOGS_ERR, "frontend framebuffer incomplete"); //TODO handle error
                        }
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        fedd.fbw = new_fbw;
                        fedd.fbh = new_fbh;
                    }
                    fedd.fex = 0;
                    fedd.fey = 0;
                    fedd.few = fedd.fbw;
                    fedd.feh = fedd.fbh;

                    glBindFramebuffer(GL_FRAMEBUFFER, fedd.frontend_fbo);
                    glViewport(0, 0, fedd.fbw, fedd.fbh);
                    metagui_empty_frontend();
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    ImGui::Image((void*)(intptr_t)fedd.frontend_tex, ImVec2(fedd.fbw, fedd.fbh), ImVec2(0, 1), ImVec2(1, 0));
                }

                /////////

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
