#include <cstdint>
#include <cstdio>

#include "imgui.h"

#include "interface/gim/window.hpp"

bool log_auto_scroll = true;
bool log_colors = true;
bool log_time = true;
bool log_status = true;

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

    if (ImGui::CollapsingHeader("Log Info")) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("line count: %lu", stored_logs.size());
        ImGui::SameLine();
        ImGui::NextColumn();
        const char* text_auto_scroll = "auto-scroll";
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text_auto_scroll).x - ImGui::GetScrollX() - 4 * ImGui::GetStyle().ItemSpacing.x);
        ImGui::Checkbox(text_auto_scroll, &log_auto_scroll);
        ImGui::Checkbox("colors", &log_colors);
        ImGui::SameLine();
        ImGui::Checkbox("status", &log_status);
        ImGui::SameLine();
        ImGui::Checkbox("time", &log_time);
        //TODO sameline + time display type (relative, absolute, etc..) combo box (but only disable if time not used)
    }

    ImGui::Separator();

    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiListClipper clipper;
    clipper.Begin(stored_logs.size());
    while (clipper.Step()) {
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
            log_entry* cur_log_entry = &stored_logs[line_no];
            bool colored = false;
            LOGS cur_status_only = (LOGS)(cur_log_entry->status & LOGS_STATUS_MASK);
            if (log_colors) {
                switch (cur_status_only) {
                    case LOGS_LESS: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
                        colored = true;
                    } break;
                    case LOGS_NORM: {
                        // pass, just normal white
                    } break;
                    case LOGS_OK: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(55, 235, 108, 255));
                        colored = true;
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
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 44, 44, 255));
                        colored = true;
                    } break;
                    //TODO other settings
                    default: {
                        // pass
                    } break;
                }
            }
            char time_display_str[32];
            //TODO switch for different modes of time display, e.g. monotonic ms ticks, ms since start, system realtime
            sprintf(time_display_str, "%09lu", cur_log_entry->time);
            ImGui::PushFont(fonts.imgui_mono);
            if (log_status) {
                const char* status_map[LOGS_COUNT] = {
                    [LOGS_LESS] = "       ",
                    [LOGS_NORM] = "     > ",
                    [LOGS_OK] = "[   OK]",
                    [LOGS_INFO] = "[ INFO]",
                    [LOGS_WARN] = "[ WARN]",
                    [LOGS_ERR] = "[ERROR]",
                    [LOGS_FATAL] = "[FATAL]",
                };
                ImGui::Text("%s%s", status_map[cur_status_only], log_time ? "" : ":");
                ImGui::SameLine();
            }
            if (log_time) {
                ImGui::Text("@%s:", time_display_str);
                ImGui::SameLine();
            }
            ImGui::TextUnformatted(cur_log_entry->msg, NULL);
            ImGui::PopFont();
            if (log_status && cur_status_only == LOGS_FATAL) {
                ImGui::Separator();
            }
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
