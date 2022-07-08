#pragma once

#pragma once

#include <stdint.h>

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_GAME_WRAP_API_VERSION = 2;

typedef struct game_wrap_s {
    const uint64_t game_api_version;
    game_methods backend;
    //TODO could also make this a struct owning its methods and options+runtime data structs, worth for this size?
    // load opts
    error_code (*opts_create)(void** options_struct);
    error_code (*opts_display)(void* options_struct);
    error_code (*opts_destroy)(void* options_struct);
    // runtime state
    error_code (*runtime_create)(game* rgame, void** runtime_struct);
    error_code (*runtime_display)(game* rgame, void* runtime_struct);
    error_code (*runtime_destroy)(game* rgame, void* runtime_struct);
} game_wrap;

#ifdef __cplusplus
}
#endif
