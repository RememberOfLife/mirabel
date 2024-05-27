#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mirabel/engine.h"

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_engine_capi_version_t)();
uint64_t plugin_get_engine_capi_version();

// always matched with a call to cleanup, may only be called again after a cleanup
void plugin_init_engine();

// writes the plugin static pointers to the engine methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_engine_methods_t)(uint32_t* count, const engine_methods** methods);
void plugin_get_engine_methods(uint32_t* count, const engine_methods** methods);

// guarantees that the plugin owner does not rely on the contents of the engine_methods anymore
// only called after init
void plugin_cleanup_engine();

#ifdef __cplusplus
}
#endif
