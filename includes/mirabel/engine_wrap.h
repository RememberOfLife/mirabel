#pragma once

#include <stdint.h>

#include "surena/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_ENGINE_WRAP_API_VERSION = 3;

typedef struct engine_wrap_s {
    const uint64_t engine_api_version;
    const engine_methods* backend;
    // load opts
    error_code (*opts_create)(void** options_struct);
    error_code (*opts_display)(void* options_struct);
    error_code (*opts_destroy)(void* options_struct);
} engine_wrap;

#ifdef __cplusplus
}
#endif
