#pragma once

#include <stdint.h>

#include "rosalia/semver.h"

#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_CLIENT_INTERFACE_API_VERSION = 1;

typedef uint32_t error_code;
static const uint32_t CLIENT_INTERFACE_ERR_OK = 0;

typedef struct client_interface_s client_interface;

typedef struct client_interface_methods_s {

    const char* name;
    const semver version;

    const char* (*get_last_error)(client_interface* self);

    //TODO supply with context ptr to use, or put that in the running interface object
    error_code (*create)(client_interface* self);

    error_code (*destroy)(client_interface* self);

    // return true to request shutdown, the interface will then only be called again once, for destruction, at some arbitrary time
    bool (*mainloop)(client_interface* self);

    //TODO log with log_id, need (un)register here too?
    void (*log)(client_interface* self, LOGS status, const char* str, const char* str_end);

    // if suggested_save_name is NULL this is a load prompt, otherwise save prompt
    const char* (*user_file_path_prompt)(client_interface* self, const char* suggested_save_name);

} client_interface_methods;

struct client_interface_s {
    const client_interface_methods* methods;
    void* data; // owned by the methods
};

#ifdef __cplusplus
}
#endif
