#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mirabel/engine_wrap.h"

//TODO
// void plugin_init_engine_wrap();

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_engine_wrap_capi_version_t)();
uint64_t plugin_get_engine_wrap_capi_version();

// writes the plugin static pointers to the engine_wrap methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_engine_wrap_methods_t)(uint32_t* count, const engine_wrap** methods);
void plugin_get_engine_wrap_methods(uint32_t* count, const engine_wrap** methods);

//TODO
// void plugin_cleanup_engine_wrap();

#ifdef __cplusplus
}
#endif
