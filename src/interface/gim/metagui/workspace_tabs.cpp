#include <cstdint>
#include <cstdio>

#include <GL/glew.h>
#include "imgui.h"

#include "mirabel/log.h"

#include "interface/gim/window.hpp"

void graphical_immediate_mode_interface::metagui_workspace_tabs()
{
    ImGui::SetNextWindowPos(imgui_viewport->WorkPos);
    ImGui::SetNextWindowSize(imgui_viewport->WorkSize);
    ImGuiWindowFlags container_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("WorkspaceTabContainer", NULL, container_flags)) {
        ImGuiTabBarFlags tabbar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable;
        if (ImGui::BeginTabBar("WorkspaceTabBar", tabbar_flags)) {

            // test for centered process tabs, e.g. connecting etc..
            bool keepcon = true;
            if (ImGui::BeginTabItem("CON", &keepcon, ImGuiTabItemFlags_NoAssumedClosure)) {

                //TODO here is where we would add the docking node for this tab, if we still want that workflow

                ImGuiIO& io = ImGui::GetIO();
                ImVec2 size = ImVec2(400, 300);
                ImVec2 window_pos = ImVec2((io.DisplaySize.x - size.x) * 0.5f, (io.DisplaySize.y - size.y) * 0.5f);
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(size);
                ImGui::Begin("intwindow", NULL, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
                ImGui::Text("GL error, frontend unavailable");
                ImGui::Button("Connect", ImVec2(-1, 0));
                ImGui::End();

                ImGui::EndTabItem();
            }

            for (int tab_idx = 0; tab_idx < 8; tab_idx++) {
                char tab_title[64];
                sprintf(tab_title, "Tab %u", tab_idx);
                bool keep = true;
                if (ImGui::BeginTabItem(tab_title, &keep, ImGuiTabItemFlags_NoAssumedClosure)) {

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
                if (!keep) {
                    //TODO user wants to close tab, show warning within tab and only close when confirmed
                }
            }
            if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing)) {
                //TODO add new tab, or pop context menu for tab opening? or just on right click?
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
