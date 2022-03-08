#include <chrono>
#include <cstdint>

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    bool show_confirm_exit_modal = false;

    bool show_main_menu_bar = true;

    bool show_stats_overlay = false;

    bool show_logs_window = false;
    const uint32_t DEBUG_LOG = 0;
    const uint32_t LOG_DEFAULT_BUFFER_SIZE = 8192;
    const std::chrono::steady_clock::time_point LOG_START_TIME = std::chrono::steady_clock::now();

    bool show_connection_window = false;

    bool show_game_config_window = false;

    bool show_frontend_config_window = false;

    bool show_engine_window = false;

}
