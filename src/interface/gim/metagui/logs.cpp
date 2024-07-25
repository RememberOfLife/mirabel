#include <cstdint>
#include <cstdio>

#include "imgui.h"

#include "interface/gim/window.hpp"

bool log_auto_scroll = true;

void graphical_immediate_mode_interface::metagui_log()
{
    if (!show_log) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
    //TODO better initial placement, docked right
    bool window_contents_visible = ImGui::Begin("Log", &show_log, ImGuiWindowFlags_NoCollapse);
    if (!window_contents_visible) {
        ImGui::End();
        return;
    }

    if (true /*ImGui::CollapsingHeader("Log Info", ImGuiTreeNodeFlags_DefaultOpen)*/) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("line count: %lu", stored_logs.size());
        ImGui::SameLine();
        ImGui::NextColumn();
        const char* text_auto_scroll = "auto-scroll";
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text_auto_scroll).x - ImGui::GetScrollX() - 4 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::Checkbox(text_auto_scroll, &log_auto_scroll);
    }

    ImGui::Separator();

    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiListClipper clipper;
    clipper.Begin(stored_logs.size());
    while (clipper.Step()) {
        for (size_t line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
            log_entry* cur_log_entry = &stored_logs[line_no];
            bool colored = false;
            switch (cur_log_entry->status & LOGS_STATUS_MASK) {
                case LOGS_LESS: {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
                    colored = true;
                } break;
                case LOGS_NORM: {
                    // pass, just normal white
                } break;
                case LOGS_OK: {

                } break;
                case LOGS_INFO: {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(44, 206, 222, 255));
                    colored = true;
                } break;
                case LOGS_WARN: {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 175, 44, 255));
                    colored = true;
                } break;
                case LOGS_ERR: {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 44, 44, 255));
                    colored = true;
                } break;
                case LOGS_FATAL: {

                } break;
                //TODO other settings
                default: {
                    // pass
                } break;
            }
            if (line_no < stored_logs.size() - 1) {
                ImGui::PushFont(fonts.imgui_mono);
                ImGui::Text("[%09lu]", cur_log_entry->time);
                ImGui::PopFont();
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(cur_log_entry->msg, NULL);
            if (colored) {
                ImGui::PopStyleColor();
            }
        }
    }
    clipper.End();
    if (log_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}
