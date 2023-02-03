#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdarg>

namespace MetaGui {

    void global_dockspace(float* x, float* y, float* w, float* h);

    extern bool show_confirm_exit_modal;
    void confirm_exit_modal(bool* p_open);

    extern bool show_main_menu_bar;
    void main_menu_bar(bool* p_open);
    extern bool fullscreen;

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

    extern bool show_config_registry_window;
    void config_registry_window(bool* p_open);

    extern bool show_connection_window;
    void connection_window(bool* p_open);

    enum RUNNING_STATE {
        RUNNING_STATE_NONE,
        RUNNING_STATE_ONGOING,
        RUNNING_STATE_DONE,
    };

    struct connection_info {
        char server_address[64];
        uint16_t server_port;
        RUNNING_STATE adapter; //TODO this might be replacable by local comparisons in the window code
        RUNNING_STATE connection;
        uint8_t* server_cert_thumbprint;
        char* verifail_reason;
        bool auth_info;
        RUNNING_STATE authentication;
        bool auth_allow_login;
        bool auth_allow_guest;
        bool auth_want_guest_pw;
        char username[32];
        char password[32];
        char* authfail_reason;
    };

    extern connection_info conn_info;
    void connection_info_reset();

    extern bool show_game_config_window;
    void game_config_window(bool* p_open); // select game, parameter config (e.g. board size), game state editing
    extern uint32_t game_base_idx;
    extern uint32_t game_variant_idx;
    extern uint32_t game_impl_idx;
    extern void* game_load_options;
    extern void* game_runtime_options;

    extern bool show_frontend_config_window;
    void frontend_config_window(bool* p_open); // select frontend to use (compatible with the game), visual config (colors)
    extern uint32_t fe_selection_idx;
    extern uint32_t selected_fem_idx;
    extern uint32_t running_fem_idx;
    extern void* frontend_load_options;

    // extern bool show_engine_window; //TODO REENABLE engine
    // void engine_window(bool* p_open); //TODO REENABLE engine

    extern bool show_chat_window;
    void chat_window(bool* p_open);
    extern bool focus_chat_input;
    void chat_msg_add(uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text); // copies the text to internals
    void chat_msg_del(uint32_t msg_id);
    void chat_clear();

    extern bool show_timectl_window;
    void timectl_window(bool* p_open);

    // extern bool show_history_window; //TODO REENABLE history
    // void history_window(bool* p_open); //TODO REENABLE history

    extern bool show_plugins_window;
    void plugins_window(bool* p_open);

    extern bool show_about_window;
    void about_window(bool* p_open);

    extern bool show_lobby_window;
    void lobby_window(bool* p_open);

    struct lobby_info {
        uint32_t id;
        char name[64];
        char password[64];
        uint16_t max_users;
    };

    extern lobby_info the_lobby_info;

    //##############################
    // format for windows is:
    // extern bool show_NAME_windowtype;
    // void NAME_windowtype(bool* p_open);
    // ... other window specific exposed functions

    // other windows we might need

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

} // namespace MetaGui
