#pragma once

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

const game_methods* load_plugin_game_methods(const char* file, uint32_t idx);

const game_methods* load_static_game_methods(const char* composite_id);

extern char cmd_prefix;

int repl();

#ifdef __cplusplus
}
#endif
