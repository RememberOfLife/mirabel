#include <stddef.h>
#include <stdlib.h>

#include "mirabel/alloc.h"

void* mirabel_malloc(size_t size)
{
    return malloc(size);
}

void* mirabel_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

void mirabel_free(void* ptr)
{
    free(ptr);
}
