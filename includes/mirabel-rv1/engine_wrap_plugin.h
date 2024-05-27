#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mirabel/engine_wrap.h"

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_engine_wrap_capi_version_t)();
uint64_t plugin_get_engine_wrap_capi_version();

// always matched with a call to cleanup, may only be called again after a cleanup
void plugin_init_engine_wrap();

// writes the plugin static pointers to the engine_wrap methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_engine_wrap_methods_t)(uint32_t* count, const engine_wrap** methods);
void plugin_get_engine_wrap_methods(uint32_t* count, const engine_wrap** methods);

// guarantees that the plugin owner does not rely on the contents of the engine_wraps anymore
// only called after init
void plugin_cleanup_engine_wrap();

#ifdef __cplusplus
}
#endif
