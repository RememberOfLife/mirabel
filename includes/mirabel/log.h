#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// thin wrapper around the metagui logging api
// use {"#I ", "#W ", "#E "} as log prefixes for importance colorization

uint32_t mirabel_log_register(const char* name);

void mirabel_log_unregister(uint32_t log_id);

void mirabel_log_clear(uint32_t log_id);

void mirabel_log(const char* str, const char* str_end);

void mirabel_logh(uint32_t log_id, const char* str, const char* str_end);

#ifdef __cplusplus
}
#endif
