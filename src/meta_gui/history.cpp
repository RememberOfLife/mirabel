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
            nvgTranslate(dc, cc*70, 50);

            // render line to child
            nvgBeginPath(dc);
            if (sel && h->selected_child == lp->idx_in_parent) {
                nvgStrokeColor(dc, nvgRGB(200, 100, 100));
            } else {
                nvgStrokeColor(dc, nvgRGB(200, 200, 200));
            }
            nvgMoveTo(dc, 0, 0);
            nvgLineTo(dc, 0, -25);
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);
            cc += 1;
            if (sel && h->selected_child == lp->idx_in_parent) {
                ccs = cc;
            }
            ccl = cc;
            cc += hm_render(dc, lp, sel && h->selected_child == lp->idx_in_parent);
            nvgRestore(dc);
            lp = lp->right_sibling;
        }

        if (cc > 0) {
            // render child bus
            nvgBeginPath(dc);
            nvgStrokeColor(dc, nvgRGB(200, 200, 200));
            nvgMoveTo(dc, 0, 25);
            nvgLineTo(dc, (ccl-1)*70, 25);
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
            nvgLineTo(dc, (ccs-1)*70, 25);
            nvgStrokeWidth(dc, 3);
            nvgStroke(dc);
        }

        // render current node
        char strbuf[50];
        sprintf(strbuf, "%lu", h->move);
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

        sprintf(strbuf, "%u/%u", h->split_height, h->width);
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
        static move_history* hr;
        if (!hr) {
            hr = (move_history*)malloc(sizeof(move_history));
            move_history_create(hr);
            hr->move = 0;
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
        if (ImGui::Button("i1")) {
            move_history_insert(current, 0, 1);
        }
        ImGui::SameLine();
        if (ImGui::Button("i2")) {
            move_history_insert(current, 0, 2);
        }
        ImGui::SameLine();
        if (ImGui::Button("i3")) {
            move_history_insert(current, 0, 3);
        }
        ImGui::SameLine();
        if (ImGui::Button("i4")) {
            move_history_insert(current, 0, 4);
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
            current->is_split = !current->is_split;
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

}
