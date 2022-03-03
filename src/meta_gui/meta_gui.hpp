#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdarg>

namespace MetaGui {

    extern bool show_confirm_exit_modal;
    void confirm_exit_modal(bool* p_open);

    extern bool show_main_menu_bar;
    void main_menu_bar(bool* p_open);

    extern bool show_stats_overlay;
    void stats_overlay(bool* p_open);

    extern bool show_logs_window;
    void logs_window(bool* p_open);
    extern const uint32_t DEBUG_LOG;
    extern const uint32_t LOG_DEFAULT_BUFFER_SIZE;
    extern const std::chrono::steady_clock::time_point LOG_START_TIME;
    void log(const char* str, const char* str_end = NULL);
    void logf(const char* fmt, ...);
    void log(uint32_t log_id, const char* str, const char* str_end = NULL);
    void logf(uint32_t log_id, const char* fmt, ...);
    void logfv(uint32_t log_id, const char* fmt, va_list args);
    // buffer_size: size of the ring buffer to use for logs, if 0 then use unlimited growing size buffer
    // returns the id of the newly registered log
    uint32_t log_register(const char* name, size_t buffer_size = LOG_DEFAULT_BUFFER_SIZE);
    void log_unregister(uint32_t log_id);
    void log_clear(uint32_t log_id);

    extern bool show_game_config_window;
    void game_config_window(bool* p_open); // select game, parameter config (e.g. board size), game state editing
    extern uint32_t base_game_idx;
    extern uint32_t game_variant_idx;

    extern bool show_frontend_config_window;
    void frontend_config_window(bool* p_open); // select frontend to use (compatible with the game), visual config (colors)
    extern uint32_t running_few_idx;
    extern uint32_t selected_few_idx;

    extern bool show_engine_window;
    void engine_window(bool* p_open);

    //##############################
    // format for windows is:
    // extern bool show_NAME_windowtype;
    // void NAME_windowtype(bool* p_open);
    // ... other window specific exposed functions

    // other windows we might need

    // "Chat"
    // - chat of the current lobby
    // - how to make text colored but copyable?
    // - a button on every message to enable copyability?!

    // "UserList"
    // - list of users (only chat or global?)
    // - moderation settings, who has game conf control in lobby etc

    // "LobbyBrowser"
    // - browse open seeks and running lobbies (to spectate if enabled)

    // "LobbyConfig"
    // - configure a lobby and launch it, or apply changes to existing lobby

    // "GameNotation"
    // - move time control
    // - game notation for players
    // - move takeback, offer draw, resign
    // - maybe textual input for game moves in notation form to be executed by the underlying game

    // player ratings list
    // - is this just a global leaderboard?
    // - or part of the user list, which may include a global directory and possibly a search function?

    // user profiles
    // - includes stats of the user on that server
    // - list: elo for different games, maybe trophies for achievements?, match history

    // seperate login window
    // - at application launch, use this to authenticate and log in to a remote server
    // - it should definitely also be possible to local host games.. maybe while still using authenticated accounts?!

    // total match list
    // - overview over all matches played on the server

    // connection window for connecting to ip and port
    // struct TextFilters
    // {
    //     // return 0 (pass) if the character is allowed
    //     static int FilterSanitizedTextLetters(ImGuiInputTextCallbackData* data)
    //     {
    //         if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", (char)data->EventChar))
    //             return 0;
    //         return 1;
    //     }
    //     static int FilterAddressLetters(ImGuiInputTextCallbackData* data)
    //     {
    //         if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-./:", (char)data->EventChar))
    //             return 0;
    //         return 1;
    //     }
    // };
    // static void GUI_ShowConnectionWindow(bool* p_open)
    // {
    //     ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    //     ImGui::Begin("Connection", p_open, window_flags);
    //     static bool connect = false;
    //     if (connect) {
    //         ImGui::BeginDisabled();
    //     }
    //     static char server_address[64] = "";
    //     ImGui::InputText("server address", server_address, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAddressLetters);
    //     static char lobby_name[64] = "";
    //     ImGui::InputText("lobby name", lobby_name, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);
    //     static bool hide_pw = true;
    //     static char password[64] = "";
    //     ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
    //     if (hide_pw) {
    //         password_flags |= ImGuiInputTextFlags_Password;
    //     }
    //     ImGui::InputText("password", password, 64, password_flags, TextFilters::FilterSanitizedTextLetters);
    //     ImGui::SameLine();
    //     if (ImGui::Button(hide_pw ? "Show" : "Hide")) {
    //         hide_pw = !hide_pw;
    //     }
    //     static char username[64] = "";
    //     ImGui::InputText("username", username, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);
    //     if (connect) {
    //         ImGui::EndDisabled();
    //     }
    //     ImGui::Separator();
    //     if (!connect) {
    //         if (ImGui::Button("Connect", ImVec2(-1.0f, 0.0f))) {
    //             connect = true;
    //         }
    //     } else {
    //         if (ImGui::Button("Cancel")) {
    //             connect = false;
    //         }
    //         ImGui::SameLine();
    //         ImGui::ProgressBar(0.5f, ImVec2(-1.0f, 0.0f));
    //     }
    //     ImGui::End();
    // }

} // namespace MetaGui
