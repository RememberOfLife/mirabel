#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(plugin_draw_t)(void);

plugin_draw_t draw;

#ifdef __cplusplus
}
#endif
