#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mirabel/frontend.h"

//TODO
// void plugin_init_frontend();

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_frontend_capi_version_t)();
uint64_t plugin_get_frontend_capi_version();

// writes the plugin static pointers to the frontend methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_frontend_methods_t)(uint32_t* count, const frontend_methods** methods);
void plugin_get_frontend_methods(uint32_t* count, const frontend_methods** methods);

//TODO
// void plugin_cleanup_frontend();

#ifdef __cplusplus
}
#endif
