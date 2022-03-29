#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "imgui.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    /*TODO FEATURES
        - some way to save logs, possibly just also write debug log to std error
        - make proper highlighting (log levels) with an enum (for the log function)
        - proper ringbuffer
            - buffer health display
        - proper timestamps (not just millis since launch) on log entries
        - ability to choose a log level for every log, that is actually displayed and filter out everything else
        - should log tabs be reorderable?
        - mechanism for managing log tab visibility
        - default logger should probably not be named "debug" might confuse with log levels later on
    */

    enum LOG_LEVEL {
        LOG_LEVEL_NONE = 0,
        LOG_LEVEL_LOG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR,
    };

    struct logger {
        std::mutex m;
        const char* name;
        size_t buffer_size;
        ImGuiTextBuffer log_buffer;
        std::vector<int> log_line_offsets;
        std::vector<std::chrono::steady_clock::time_point> log_line_timestamps;
        bool auto_scroll = true;
        LOG_LEVEL dirty = LOG_LEVEL_NONE;
        bool visible = false;
        logger();
        logger(const char* name, size_t buffer_size);
        void show();
        void log(const char* str, const char* str_end = NULL);
        void logfv(const char* fmt, va_list args);
        void clear();
    };

    logger::logger()
    {}

    logger::logger(const char* name, size_t buffer_size):
        name(name),
        buffer_size(buffer_size)
    {
        log_line_offsets.push_back(0);
    }

    void logger::show()
    {
        m.lock();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        const char* buf = log_buffer.begin();
        const char* buf_end = log_buffer.end();
        ImGuiListClipper clipper;
        clipper.Begin(log_line_offsets.size());
        while (clipper.Step())
        {
            for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
            {
                const char* line_start = buf + log_line_offsets[line_no];
                const char* line_end = (line_no + 1 < log_line_offsets.size()) ? (buf + log_line_offsets[line_no + 1] - 1) : buf_end;
                bool colored = false;
                if (*line_start == '#') {
                    colored = true;
                    switch (*(line_start+1)) {
                        default: {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255));
                        } break;
                        case 'I': {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(44, 206, 222, 255));
                        } break;
                        case 'W': {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 175, 44, 255));
                        } break;
                        case 'E': {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 44, 44, 255));
                        } break;
                    }
                    if (line_end-line_start > 3) {
                        line_start += 3;
                    }
                }
                if (line_no < log_line_offsets.size()-1) {
                    ImGui::Text("[%09lu]", std::chrono::duration_cast<std::chrono::milliseconds>(log_line_timestamps[line_no]-LOG_START_TIME).count());
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(line_start, line_end);
                if (colored) {
                    ImGui::PopStyleColor();
                }
            }
        }
        clipper.End();
        if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        m.unlock();
    }

    void logger::log(const char* str, const char* str_end)
    {
        m.lock();
        int old_size = log_buffer.size();
        log_buffer.append(str, str_end);
        for (int new_size = log_buffer.size(); old_size < new_size; old_size++) {
            if (log_buffer[old_size] == '\n') {
                log_line_offsets.push_back(old_size + 1);
                log_line_timestamps.push_back(std::chrono::steady_clock::now());
            }
        }
        int line_offset = log_line_offsets.size()-2;
        if (line_offset < 0) {
            line_offset = 0;
        } 
        const char* line_start = log_buffer.begin()+(log_line_offsets[line_offset]);
        LOG_LEVEL update_dirty = LOG_LEVEL_NONE;
        if (*line_start == '#') {
            switch (*(line_start+1)) {
                case 'I': {
                    update_dirty = LOG_LEVEL_INFO;
                } break;
                case 'W': {
                    update_dirty = LOG_LEVEL_WARNING;
                } break;
                case 'E': {
                    update_dirty = LOG_LEVEL_ERROR;
                } break;
            }
        }
        if (!visible && update_dirty > dirty) {
            dirty = update_dirty;
        }
        m.unlock();
    }
    
    void logger::logfv(const char* fmt, va_list args)
    {
        m.lock();
        int old_size = log_buffer.size();
        log_buffer.appendfv(fmt, args);
        for (int new_size = log_buffer.size(); old_size < new_size; old_size++) {
            if (log_buffer[old_size] == '\n') {
                log_line_offsets.push_back(old_size + 1);
                log_line_timestamps.push_back(std::chrono::steady_clock::now());
            }
        }
        int line_offset = log_line_offsets.size()-2;
        if (line_offset < 0) {
            line_offset = 0;
        } 
        const char* line_start = log_buffer.begin()+(log_line_offsets[line_offset]);
        LOG_LEVEL update_dirty = LOG_LEVEL_LOG;
        if (*line_start == '#') {
            switch (*(line_start+1)) {
                case 'I': {
                    update_dirty = LOG_LEVEL_INFO;
                } break;
                case 'W': {
                    update_dirty = LOG_LEVEL_WARNING;
                } break;
                case 'E': {
                    update_dirty = LOG_LEVEL_ERROR;
                } break;
            }
        }
        if (!visible && update_dirty > dirty) {
            dirty = update_dirty;
        }
        m.unlock();
    }

    void logger::clear()
    {
        m.lock();
        log_buffer.clear();
        log_line_offsets.clear();
        log_line_offsets.push_back(0);
        log_line_timestamps.clear();
        m.unlock();
    }

    static uint32_t next_log_id = DEBUG_LOG+1;
    static std::unordered_map<uint32_t, logger*> logs = {{DEBUG_LOG, new logger("DEBUG", LOG_DEFAULT_BUFFER_SIZE)}};
    static std::vector<uint32_t> visible_logs = {DEBUG_LOG}; // this keeps the order of the visible logs
    static uint32_t visible_log = DEBUG_LOG;

    bool log_exists(uint32_t log_id)
    {
        return logs.find(log_id) != logs.end();
    }

    bool log_exists(const char* name)
    {
        for (int i = 0; i < visible_logs.size(); i++) {
            if (strcmp(logs[visible_logs[i]]->name, name) == 0) {
                return true;
            }
        }
        return false;
    }

    void logs_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Logs", p_open);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }
        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            //TODO fix for closed elsewhere glitch
            // submit Tabs
            for (int log_n = 0; log_n < visible_logs.size(); log_n++)
            {
                uint32_t current_log_id = visible_logs[log_n];
                logger* current_log = logs[current_log_id];
                switch (current_log->dirty) {
                    case LOG_LEVEL_NONE: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(190, 190, 190, 255));
                    } break;
                    case LOG_LEVEL_LOG: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 250, 250, 255));
                    } break;
                    case LOG_LEVEL_INFO: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(44, 206, 222, 255));
                    } break;
                    case LOG_LEVEL_WARNING: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 175, 44, 255));
                    } break;
                    case LOG_LEVEL_ERROR: {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(222, 44, 44, 255));
                    } break;
                }
                ImGui::PushID(current_log_id);
                bool visible = ImGui::BeginTabItem(current_log->name);
                ImGui::PopStyleColor();
                current_log->visible = visible;
                if (visible) {
                    visible_log = current_log_id;

                    // collapsing header with info + settings about the log
                    if (ImGui::CollapsingHeader("Logger", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("line count: %lu", current_log->log_line_offsets.size()-1);
                        ImGui::SameLine();
                        ImGui::NextColumn();
                        const char* text_auto_scroll = "auto-scroll";
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(text_auto_scroll).x - ImGui::GetScrollX() - 3 * ImGui::GetStyle().ItemSpacing.x);
                        ImGui::Checkbox(text_auto_scroll, &current_log->auto_scroll);
                        ImGui::Separator();
                    }
                    current_log->show();
                    current_log->dirty = LOG_LEVEL_NONE;
                    
                    ImGui::EndTabItem();
                }
                ImGui::PopID();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    void log(const char* str, const char* str_end)
    {
        log(DEBUG_LOG, str, str_end);
    }
    
    void logf(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logfv(DEBUG_LOG, fmt, args);
        va_end(args);
    }

    void log(uint32_t log_id, const char* str, const char* str_end)
    {
        if (log_exists(log_id)) {
            logs[log_id]->log(str, str_end);
        }
    }
    
    void logf(uint32_t log_id, const char* fmt, ...)
    {
        if (log_exists(log_id)) {
            va_list args;
            va_start(args, fmt);
            logs[log_id]->logfv(fmt, args);
            va_end(args);
        }
        
    }
    
    void logfv(uint32_t log_id, const char* fmt, va_list args)
    {
        if (log_exists(log_id)) {
            logs[log_id]->logfv(fmt, args);
        }
    }

    uint32_t log_register(const char* name, std::size_t buffer_size)
    {
        if (log_exists(name)) {
            logf("#W registering logger#%d with an already existing name: %s\n", next_log_id, name);
        }
        logf("#I registering logger#%d: %s\n", next_log_id, name);
        logs[next_log_id] = new logger(name, buffer_size);
        visible_logs.push_back(next_log_id);
        return next_log_id++;
    }

    void log_unregister(uint32_t log_id)
    {
        if (log_id == DEBUG_LOG) {
            log("#W attempted un-register of debug logger\n");
            return;
        }
        if (!log_exists(log_id)) {
            logf("#W attempted un-register of unknown logger#%d\n", log_id);
            return;
        }
        logs[log_id]->clear();
        logs.erase(log_id);
        visible_logs.erase(std::remove(visible_logs.begin(), visible_logs.end(), log_id), visible_logs.end());
        if (visible_log == log_id) {
            visible_log = DEBUG_LOG;
        }
        logf("#I un-registered logger#%d\n", log_id);
    }

    void log_clear(uint32_t log_id)
    {
        if (!log_exists(log_id)) {
            logf("#W attempted clear of unknown logger#%d\n", log_id);
            return;
        }
        logs[log_id]->clear();
        logf("#I cleared logger#%d: %s\n", log_id, logs[log_id]->name);
    }

}
