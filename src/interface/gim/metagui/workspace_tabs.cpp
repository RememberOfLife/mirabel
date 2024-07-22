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
    ImGui::Begin("testimg window");
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    float new_fbw = content_size.x;
    float new_fbh = content_size.y;
    if (new_fbw <= 0 || new_fbh <= 0) {
        return; // dont draw if invisible, otherwise FBO errors
    }
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
    metagui_empty_frontend();
    ImGui::Image((void*)(intptr_t)fedd.frontend_tex, ImVec2(fedd.fbw, fedd.fbh), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
    return;
    ImGui::SetNextWindowPos(imgui_viewport->WorkPos);
    ImGui::SetNextWindowSize(imgui_viewport->WorkSize);
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    if (ImGui::Begin("WorkspaceTabContainer", NULL, flags)) {

        static ImGuiWindowFlags flags_tab_bar = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::BeginChild("TabBarContainer")) {
            if (ImGui::BeginTabBar("WorkspaceTabBar")) {
                if (ImGui::BeginTabItem("Tab 1")) {
                    ImGui::Text("1111111111111111111111");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Tab 2")) {
                    ImGui::Text("2222222222222222222222");
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Tab 3")) {
                    ImGui::Text("3333333333333333333333");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // ImVec2 size = ImGui::GetContentRegionAvail();
        // ImGui::BeginChild("OpenGLRenderArea1", size, false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        // ImVec2 render_pos = ImGui::GetCursorScreenPos();
        // ImVec2 render_size = ImGui::GetContentRegionAvail();
        // fedd.fex = render_pos.x;
        // fedd.fey = render_pos.y;
        // fedd.few = render_size.x;
        // fedd.feh = render_size.y;
        // metagui_empty_frontend();
        // // disable ImGui inputs in this region
        // ImGui::SetItemAllowOverlap();
        // ImGui::EndChild();
    }
    ImGui::End();
}
