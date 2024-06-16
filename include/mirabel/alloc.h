#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* mirabel_malloc(size_t size);
void* mirabel_realloc(void* ptr, size_t size);
void mirabel_free(void* ptr);

#ifdef __cplusplus
}
#endif
