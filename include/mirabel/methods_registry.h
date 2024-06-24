#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct methods_entry_s {
    uint32_t type_hash;
    uint32_t name_hash;
    char* methods_type;
    char* methods_name;
    const void* methods;
} methods_entry;

typedef struct methods_registry_s {
    /*TODO future design
    typedef struct method_type_s {
        ROSA_TREEMAP(NOR, ROSA_STRKEY, void* method);
    } method_type;
    ROSA_TREEMAP(NOR, ROSA_STRKEY, method_type);
    */

    methods_entry* entries; // rosa_vec
} methods_registry;

void methods_registry_create(methods_registry* reg);

void methods_registry_destroy(methods_registry* reg);

bool methods_registry_add(methods_registry* reg, const char* methods_type, const char* methods_name, const void* methods); // returns true if successfully added

const void* methods_registry_get(methods_registry* reg, const char* methods_type, const char* methods_name); // NULL if non-existant

bool methods_registry_remove(methods_registry* reg, const char* methods_type, const char* methods_name); // returns true if successfully removed

#ifdef __cplusplus
}
#endif
