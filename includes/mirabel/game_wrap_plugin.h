#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mirabel/game_wrap.h"

//TODO
// void plugin_init_game_wrap();

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_game_wrap_capi_version_t)();
uint64_t plugin_get_game_wrap_capi_version();

// writes the plugin static pointers to the game_wrap methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_game_wrap_methods_t)(uint32_t* count, const game_wrap** methods);
void plugin_get_game_wrap_methods(uint32_t* count, const game_wrap** methods);

//TODO
// void plugin_cleanup_game_wrap();

#ifdef __cplusplus
}
#endif
