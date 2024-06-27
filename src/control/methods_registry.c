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

// usage:
// str, str, ignored => find that method
// str, null, find_fail => count all of the type
// str, null, n => find n'th method of that type
uint32_t methods_registry_find_internal(methods_registry* reg, const char* methods_type, const char* methods_name, uint32_t skip_count)
{
    uint32_t type_hash = strhash(methods_type, NULL);
    uint32_t name_hash = methods_name != NULL ? strhash(methods_name, NULL) : 0;
    uint32_t found_count = 0;
    for (uint32_t i = 0; i < VEC_LEN(&reg->entries); i++) {
        methods_entry* e = &reg->entries[i];
        bool found = true;
        found &= (e->type_hash == type_hash && strcmp(e->methods_type, methods_type) == 0);
        if (methods_name != NULL) {
            found &= (e->name_hash == name_hash && strcmp(e->methods_name, methods_name) == 0);
        } else {
            if (found) {
                found_count++;
            }
            found &= (found_count - 1 == skip_count);
        }
        if (found) {
            return i;
        }
    }
    if (methods_name == NULL && skip_count == METHODS_REGISTRY_FIND_FAIL) {
        return found_count;
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
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name, 0);
    if (idx != METHODS_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "methods registry: add failed, \"%s\".\"%s\", already exists (current: %p, requested: %p)", methods_type, methods_name, reg->entries[idx].methods, methods);
        return true;
    }
    if (methods_type == NULL || methods_name == NULL || methods == NULL) {
        mirabel_slogf(LOGS_ERR, "methods registry: add failed, \"%s\".\"%s\", type and name and methods may not be null");
        return true;
    }
    VEC_PUSH_N(&reg->entries, 1);
    VEC_LAST(&reg->entries) = (methods_entry){
        .type_hash = strhash(methods_type, NULL),
        .name_hash = strhash(methods_name, NULL),
        .methods_type = strdup(methods_type),
        .methods_name = strdup(methods_name),
        .methods = methods,
    };
    return false;
}

bool methods_registry_remove(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name, 0);
    if (idx == METHODS_REGISTRY_FIND_FAIL) {
        mirabel_slogf(LOGS_ERR, "methods registry: remove failed, \"%s\".\"%s\" does not exist", methods_type, methods_name);
        return true;
    }
    free(reg->entries[idx].methods_type);
    free(reg->entries[idx].methods_name);
    VEC_REMOVE_SWAP(&reg->entries, idx);
    return false;
}

const void* methods_registry_get(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name, 0);
    if (idx == METHODS_REGISTRY_FIND_FAIL) {
        return NULL;
    }
    return reg->entries[idx].methods;
}

const methods_entry* methods_registry_get_entry(methods_registry* reg, const char* methods_type, const char* methods_name)
{
    uint32_t idx = methods_registry_find_internal(reg, methods_type, methods_name, 0);
    if (idx == METHODS_REGISTRY_FIND_FAIL) {
        return NULL;
    }
    return &reg->entries[idx];
}

uint32_t methods_registry_get_count(methods_registry* reg, const char* method_type)
{
    return methods_registry_find_internal(reg, method_type, NULL, METHODS_REGISTRY_FIND_FAIL);
}

const methods_entry* methods_registry_get_entry_by_idx(methods_registry* reg, const char* method_type, uint32_t idx)
{
    uint32_t internal_idx = methods_registry_find_internal(reg, method_type, NULL, idx);
    if (internal_idx == METHODS_REGISTRY_FIND_FAIL) {
        return NULL;
    }
    return &reg->entries[internal_idx];
}
