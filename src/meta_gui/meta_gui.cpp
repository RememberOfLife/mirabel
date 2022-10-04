#include <chrono>
#include <cstdint>

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    bool show_confirm_exit_modal = false;

    bool show_main_menu_bar = true;

    bool show_stats_overlay = false;

    bool show_logs_window = true; //DEBUG
    const uint32_t DEBUG_LOG = 0;
    const uint32_t LOG_DEFAULT_BUFFER_SIZE = 8192;
    const std::chrono::steady_clock::time_point LOG_START_TIME = std::chrono::steady_clock::now();

    bool show_config_registry_window = false;

    bool show_connection_window = false;
    //BUG gcc bug makes this ugly for now https://stackoverflow.com/questions/70172941/c99-designator-member-outside-of-aggregate-initializer
    connection_info conn_info = connection_info{
        /*.server_address = */ "127.0.0.1", //TODO for debugging purposes this is loopback
        .server_port = 61801, // default mirabel port
        .adapter = RUNNING_STATE_NONE,
        .connection = RUNNING_STATE_NONE,
        .server_cert_thumbprint = NULL,
        .verifail_reason = NULL,
        .auth_info = false,
        .authentication = RUNNING_STATE_NONE,
        .auth_allow_login = false,
        .auth_allow_guest = false,
        .auth_want_guest_pw = false,
        /*.username = */ "",
        /*.password = */ "",
        .authfail_reason = NULL,
    };

    bool show_game_config_window = false;

    bool show_frontend_config_window = false;

    bool show_engine_window = false;

    bool show_chat_window = false;

    bool show_timectl_window = false;

    bool show_history_window = false;

    bool show_plugins_window = false;

    bool show_about_window = false;

} // namespace MetaGui
