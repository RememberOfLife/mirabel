#pragma once

#pragma once

#include <stdint.h>

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_GAME_WRAP_API_VERSION = 6;

typedef struct game_wrap_feature_flags_s {
    bool options : 1;
    bool runtime : 1;
} game_wrap_feature_flags;

typedef struct game_wrap_s {
    const uint64_t game_api_version;
    const game_methods* backend;
    const game_wrap_feature_flags features;

    //TODO could also make this a struct owning its methods and options+runtime data structs, worth for this size?

    // FEATURE: options
    // load opts, will also be used if the backend does not support options and wrapper supports initial state
    error_code (*opts_create)(void** options_struct);
    error_code (*opts_display)(void* options_struct);
    error_code (*opts_destroy)(void* options_struct);

    // for both: if str_buf is NULL, set ret_size to required size, otherwise it is ignored!
    // FEATURE: !backend.options_bin
    error_code (*opts_bin_to_str)(void* options_struct, char* str_buf, size_t* ret_size); 

    //TODO want this? or a more general approach where the wrap gets the whole game? or even at all?
    // FEATURE: options && initial_state
    // error_code (*opts_initial_state)(void* options_struct, char* str_buf, size_t* ret_size);

    // FEATURE: runtime
    // runtime state, created when the game is created, destroyed when the game is destroyed
    //TODO need game step to notice updates
    error_code (*runtime_create)(game* rgame, void** runtime_struct);
    error_code (*runtime_display)(game* rgame, void* runtime_struct);
    error_code (*runtime_destroy)(void* runtime_struct);
    //TODO this needs some way to issue state updates, just the client inbox, or a dedicated struct like the frontend display data?

} game_wrap;

#ifdef __cplusplus
}
#endif
