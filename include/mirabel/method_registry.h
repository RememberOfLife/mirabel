#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct method_entry_s {
    uint32_t type_hash;
    uint32_t name_hash;
    char* method_type;
    char* method_name;
    void* method;
} method_entry;

typedef struct method_registry_s {
    /*TODO future design
    typedef struct method_type_s {
        ROSA_TREEMAP(NOR, ROSA_STRKEY, void* method);
    } method_type;
    ROSA_TREEMAP(NOR, ROSA_STRKEY, method_type);
    */

    method_entry* entries; // rosa_vec
} method_registry;

method_registry* method_registry_create();

void method_registry_destroy(method_registry* reg);

bool method_registry_add(method_registry* reg, const char* method_type, const char* method_name, void* method); // returns true if successfully added

void* method_registry_get(method_registry* reg, const char* method_type, const char* method_name); // NULL if non-existant

bool method_registry_remove(method_registry* reg, const char* method_type, const char* method_name); // returns true if successfully removed

#ifdef __cplusplus
}
#endif
