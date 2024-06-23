#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "rosalia/noise.h"
#include "rosalia/vector.h"

#include "mirabel/alloc.h"
#include "mirabel/log.h"
#include "mirabel/method_registry.h"

/////
// internal

static const uint32_t METHOD_REGISTRY_FIND_FAIL = UINT32_MAX;

uint32_t method_registry_find_internal(method_registry* reg, const char* method_type, const char* method_name)
{
    uint32_t type_hash = strhash(method_type, NULL);
    uint32_t name_hash = strhash(method_name, NULL);
    for (uint32_t i = 0; i < VEC_LEN(&reg->entries); i++) {
        method_entry* e = &reg->entries[i];
        if (
            e->type_hash == type_hash &&
            strcmp(e->method_type, method_type) == 0 &&
            e->name_hash == name_hash &&
            strcmp(e->method_name, method_name) == 0
        ) {
            return i;
        }
    }
    return METHOD_REGISTRY_FIND_FAIL;
}

/////
// public

void method_registry_create(method_registry* reg)
{
    VEC_CREATE(&reg->entries, 0);
}

void method_registry_destroy(method_registry* reg)
{
    VEC_DESTROY(&reg->entries);
}

bool method_registry_add(method_registry* reg, const char* method_type, const char* method_name, void* method)
{
    uint32_t idx = method_registry_find_internal(reg, method_type, method_name);
    if (idx == METHOD_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "method registry: add failed, \"%s\".\"%s\", already exists (current: %p, requested: %p)", method_type, method_name, reg->entries[idx].method, method);
        return false;
    }
    if (method_type == NULL || method_name == NULL || method == NULL) {
        mirabel_slogf(LOGS_ERR, "method registry: add failed, \"%s\".\"%s\", type and name and method may not be null");
        return false;
    }
    VEC_PUSH_N(&reg->entries, 1);
    VEC_LAST(&reg->entries) = (method_entry){
        .type_hash = strhash(method_type, NULL),
        .name_hash = strhash(method_name, NULL),
        .method_type = strdup(method_type),
        .method_name = strdup(method_name),
        .method = method,
    };
    return true;
}

void* method_registry_get(method_registry* reg, const char* method_type, const char* method_name)
{
    uint32_t idx = method_registry_find_internal(reg, method_type, method_name);
    if (idx == METHOD_REGISTRY_FIND_FAIL) {
        return NULL;
    }
    return reg->entries[idx].method;
}

bool method_registry_remove(method_registry* reg, const char* method_type, const char* method_name)
{
    uint32_t idx = method_registry_find_internal(reg, method_type, method_name);
    if (idx == METHOD_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "method registry: remove failed, \"%s\".\"%s\" does not exist", method_type, method_name);
        return false;
    }
    free(reg->entries[idx].method_type);
    free(reg->entries[idx].method_name);
    VEC_REMOVE_SWAP(&reg->entries, idx);
    return true;
}
