#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "rosalia/noise.h"
#include "rosalia/vector.h"

#include "mirabel/log.h"

#include "mirabel/methods_registry.h"

/////
// internal

static const uint32_t METHODS_REGISTRY_FIND_FAIL = UINT32_MAX;

uint32_t methods_registry_find_internal(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t type_hash = strhash(methods_type, NULL);
    uint32_t name_hash = strhash(methods_name, NULL);
    for (uint32_t i = 0; i < VEC_LEN(&reg->entries); i++) {
        methods_entry* e = &reg->entries[i];
        if (
            e->type_hash == type_hash &&
            strcmp(e->methods_type, methods_type) == 0 &&
            e->name_hash == name_hash &&
            strcmp(e->methods_name, methods_name) == 0
        ) {
            return i;
        }
    }
    return METHODS_REGISTRY_FIND_FAIL;
}

/////
// public

void methods_registry_create(methods_registry* reg)
{
    VEC_CREATE(&reg->entries, 0);
}

void methods_registry_destroy(methods_registry* reg)
{
    VEC_DESTROY(&reg->entries);
}

bool methods_registry_add(methods_registry* reg, const char* methods_type, const char* methods_name, const void* methods)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name);
    if (idx != METHODS_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "methods registry: add failed, \"%s\".\"%s\", already exists (current: %p, requested: %p)", methods_type, methods_name, reg->entries[idx].methods, methods);
        return false;
    }
    if (methods_type == NULL || methods_name == NULL || methods == NULL) {
        mirabel_slogf(LOGS_ERR, "methods registry: add failed, \"%s\".\"%s\", type and name and methods may not be null");
        return false;
    }
    VEC_PUSH_N(&reg->entries, 1);
    VEC_LAST(&reg->entries) = (methods_entry){
        .type_hash = strhash(methods_type, NULL),
        .name_hash = strhash(methods_name, NULL),
        .methods_type = strdup(methods_type),
        .methods_name = strdup(methods_name),
        .methods = methods,
    };
    return true;
}

const void* methods_registry_get(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name);
    if (idx == METHODS_REGISTRY_FIND_FAIL) {
        return NULL;
    }
    return reg->entries[idx].methods;
}

bool methods_registry_remove(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name);
    if (idx == METHODS_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "methods registry: remove failed, \"%s\".\"%s\" does not exist", methods_type, methods_name);
        return false;
    }
    free(reg->entries[idx].methods_type);
    free(reg->entries[idx].methods_name);
    VEC_REMOVE_SWAP(&reg->entries, idx);
    return true;
}
