#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/move_history.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    int hm_render(NVGcontext* dc, move_history* h, bool sel)
    {
        // render children
        move_history* lp = h->left_child;
        int cc = 0;
        int ccl = 0;
        int ccs = UINT32_MAX;
        while (lp) {
            nvgSave(dc);
            nvgTranslate(dc, cc * 70, 50);
            // render line to child
            nvgBeginPath(dc);
            if (sel && h->selected_child == lp->idx_in_parent) {
                nvgStrokeColor(dc, nvgRGB(200, 100, 100));
            } else {
                nvgStrokeColor(dc, nvgRGB(200, 200, 200));
            }
            nvgMoveTo(dc, 0, 0);
            nvgLineTo(dc, 0, -25);
            if (lp->parent && lp->parent->is_split && lp->idx_in_parent == 0) {
                nvgLineTo(dc, 0, 50 * lp->parent->split_height);
            }
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);
            if (lp->parent && lp->parent->is_split && lp->idx_in_parent == 0) {
                nvgTranslate(dc, 0, 50 * lp->parent->split_height);
            }
            cc += 1;
            if (sel && h->selected_child == lp->idx_in_parent) {
                ccs = cc;
            }
            ccl = cc;
            if (lp->parent && lp->parent->is_split && lp->idx_in_parent == 0) {
                hm_render(dc, lp, sel && h->selected_child == lp->idx_in_parent);
            } else {
                cc += hm_render(dc, lp, sel && h->selected_child == lp->idx_in_parent);
            }
            nvgRestore(dc);
            lp = lp->right_sibling;
        }

        if (cc > 0) {
            // render child bus
            nvgBeginPath(dc);
            nvgStrokeColor(dc, nvgRGB(200, 200, 200));
            nvgMoveTo(dc, 0, 25);
            nvgLineTo(dc, (ccl - 1) * 70, 25);
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);

            // render line from own to child bus
            nvgBeginPath(dc);
            if (ccs != UINT32_MAX) {
                nvgStrokeColor(dc, nvgRGB(200, 100, 100));
            } else {
                nvgStrokeColor(dc, nvgRGB(200, 200, 200));
            }
            nvgMoveTo(dc, 0, 0);
            nvgLineTo(dc, 0, 25);
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);
        }

        if (ccs != UINT32_MAX) {
            // render highlighted bus to selected child
            nvgBeginPath(dc);
            nvgStrokeColor(dc, nvgRGB(200, 100, 100));
            nvgMoveTo(dc, 0, 25);
            nvgLineTo(dc, (ccs - 1) * 70, 25);
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);
        }

        // render current node
        char strbuf[50];
        sprintf(strbuf, "%lu", h->move.md.cl.code);
        nvgBeginPath(dc);
        nvgRect(dc, -25, -15, 50, 30);
        if (sel) {
            nvgFillColor(dc, nvgRGB(200, 100, 100));
        } else {
            nvgFillColor(dc, nvgRGB(200, 200, 200));
        }
        nvgFill(dc);

        nvgBeginPath(dc);
        nvgFillColor(dc, nvgRGB(0, 0, 0));
        nvgFontFace(dc, "df");
        nvgFontSize(dc, 20);
        nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(dc, 0, 0, strbuf, NULL);

        sprintf(strbuf, "%u/%u/%u", h->height, h->split_height, h->width);
        nvgBeginPath(dc);
        nvgFillColor(dc, nvgRGB(0, 0, 0));
        nvgFontFace(dc, "df");
        nvgFontSize(dc, 12);
        nvgTextAlign(dc, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(dc, 0, 15, strbuf, NULL);

        nvgFill(dc);
        nvgBeginPath(dc);
        if (h->is_split) {
            nvgFillColor(dc, nvgRGB(95, 255, 24));
        } else {
            nvgFillColor(dc, nvgRGB(128, 128, 128));
        }
        nvgCircle(dc, -20, -10, 4);
        nvgFill(dc);

        return cc > 0 ? cc - 1 : 0;
    }

    void history_mockup(NVGcontext* dc)
    {
        //TODO for now this is mockup render directly onto the frontend canvas
        static move_history* hr = NULL;
        if (!hr) {
            hr = move_history_create();
            //TODO INTEGRATION set move to none
            hr->move.md.cl.code = 0;
        }
        move_history* current = hr;
        while (true) {
            uint32_t selc = current->selected_child;
            if (selc == UINT32_MAX) {
                break;
            }
            current = current->left_child;
            for (int i = 0; i < selc; i++) {
                current = current->right_sibling;
            }
        }

        nvgSave(dc);
        nvgTranslate(dc, 70, 50);
        hm_render(dc, hr, true);
        nvgRestore(dc);

        ImGui::Begin("history mockup");
        move_data_sync sync_move = {
            .sync_ctr = 0,
        };
        if (ImGui::Button("i1")) {
            sync_move.md.cl.code = 1;
            move_history_insert(current, NULL, 0, sync_move, "1");
        }
        ImGui::SameLine();
        if (ImGui::Button("i2")) {
            sync_move.md.cl.code = 2;
            move_history_insert(current, NULL, 0, sync_move, "2");
        }
        ImGui::SameLine();
        if (ImGui::Button("i3")) {
            sync_move.md.cl.code = 3;
            move_history_insert(current, NULL, 0, sync_move, "3");
        }
        ImGui::SameLine();
        if (ImGui::Button("i4")) {
            sync_move.md.cl.code = 4;
            move_history_insert(current, NULL, 0, sync_move, "4");
        }
        if (ImGui::Button("select root")) {
            move_history_select(hr);
        }
        if (ImGui::Button("select up")) {
            if (current->parent) {
                move_history_select(current->parent);
            }
        }
        if (ImGui::Button("t split")) {
            move_history_split(current, !current->is_split);
        }
        if (ImGui::Button("promote main")) {
            move_history_promote(current, true);
        }
        if (ImGui::Button("promote")) {
            move_history_promote(current, false);
        }
        if (ImGui::Button("demote")) {
            move_history_demote(current);
        }
        if (ImGui::Button("cut")) {
            if (current != hr) {
                move_history_destroy(current);
            }
        }
        ImGui::End();
    }

    void history_window(bool* p_open)
    {
        //TODO
    }

    /* two ways of doing history, one with gl texture and another with imgui cursors placements
    static bool inted = false;
    static GLuint renderedTexture;
    static GLuint FramebufferName = 0;
    if (!inted) {
        inted = true;
        glGenFramebuffers(1, &FramebufferName);
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

        // The texture we're going to render to
        glGenTextures(1, &renderedTexture);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, renderedTexture);

        // Give an empty image to OpenGL ( the last "0" )
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dd.fbw, dd.fbh, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        // Poor filtering. Needed !
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
    }
    ImVec2 pos;
    ImVec2 wsize;
    static bool a = true;
    if (ImGui::Begin("hist", &a, ImGuiWindowFlags_HorizontalScrollbar) && a) {
        // Using a Child allow to fill all the space of the window.
        // It also alows customization
        ImGui::BeginChild("GameRender");
        pos = ImGui::GetCursorScreenPos();
        // Get the size of the child (i.e. the whole draw size of the windows).
        wsize = ImGui::GetWindowSize();

        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
        glViewport(0, 0, wsize.x, wsize.y); // Render on the whole framebuffer, complete from the lower left corner to the upper right

        // imgui renders our nanovg image
        NVGcontext* dc = Control::main_client->nanovg_ctx;
        nvgBeginFrame(dc, wsize.x, wsize.y, 2);

        nvgBeginPath(dc);
        nvgFillColor(dc, nvgRGB(255, 255, 255));
        nvgRect(dc, -10, -10, wsize.x + 20, wsize.y + 20);
        nvgFill(dc);

        nvgBeginPath(dc);
        nvgFillColor(dc, nvgRGB(0, 0, 255));
        nvgRect(dc, 10, 10, wsize.x - 20, wsize.y - 20);
        nvgFill(dc);
        // MetaGui::history_mockup(Control::main_client->nanovg_ctx);
        nvgEndFrame(dc);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // give texture to imgui
        // ImGui::GetWindowDrawList()->AddImage(
        //     (void*)(size_t)renderedTexture,
        //     ImVec2(pos.x, pos.y),
        //     ImVec2(pos.x + wsize.x, pos.y + wsize.y),
        //     ImVec2(0, 0),
        //     ImVec2(wsize.x / dd.fbw, wsize.y / dd.fbh)
        // );

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 col = ImColor(156, 156, 156);
        draw_list->AddLine(ImVec2(pos.x + 410, pos.y + 410), ImVec2(pos.x + 410, pos.y + 460), col, 4);
        draw_list->AddLine(ImVec2(pos.x + 410, pos.y + 435), ImVec2(pos.x + 560, pos.y + 435), col, 4);
        col = ImColor(214, 103, 103);
        draw_list->AddLine(ImVec2(pos.x + 510, pos.y + 410), ImVec2(pos.x + 510, pos.y + 460), col, 4);

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(126, 126, 126, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));

        ImGui::SetCursorPos(ImVec2(400, 400));
        ImGui::Button("abc");
        ImGui::SetCursorPos(ImVec2(450, 400));
        ImGui::Button("abcd");
        ImGui::SetCursorPos(ImVec2(500, 400));
        ImGui::Button("abce");
        ImGui::SetCursorPos(ImVec2(550, 400));
        ImGui::Button("abcf");

        ImGui::PopStyleColor(3);

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(247, 80, 80, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(247, 80, 80, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(247, 80, 80, 255));

        ImGui::SetCursorPos(ImVec2(400, 450));
        ImGui::Button("abcg");
        ImGui::SetCursorPos(ImVec2(450, 450));
        ImGui::Button("abch");

        ImGui::PopStyleColor(3);

        col = ImColor(97, 255, 129);
        draw_list->AddCircleFilled(ImVec2(pos.x + 452, pos.y + 452), 5, col, 10);

        ImGui::EndChild();
    }
    ImGui::End();
    */

} // namespace MetaGui
