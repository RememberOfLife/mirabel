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

typedef const char* client_interface_get_last_error_t(client_interface* self);

//TODO supply with context ptr to use, or put that in the running interface object
// if this fails, call destroy
typedef error_code client_interface_create_t(client_interface* self);

typedef void client_interface_destroy_t(client_interface* self);

// return true to request shutdown, the interface will then only be called again once, for destruction, at some arbitrary time
typedef bool client_interface_mainloop_t(client_interface* self);

//TODO log with log_id, need (un)register here too?
typedef void client_interface_log_t(client_interface* self, LOGS status, const char* str, const char* str_end);

// if suggested_save_name is NULL this is a load prompt, otherwise save prompt
typedef const char* client_interface_user_file_path_prompt_t(client_interface* self, const char* suggested_save_name);

typedef struct client_interface_methods_s {

    const char* name;
    const semver version;

    client_interface_get_last_error_t* get_last_error;
    client_interface_create_t* create;
    client_interface_destroy_t* destroy;
    client_interface_mainloop_t* mainloop;
    client_interface_log_t* log;
    client_interface_user_file_path_prompt_t* user_file_path_prompt;

} client_interface_methods;

struct client_interface_s {
    const client_interface_methods* methods;
    void* data; // owned by the methods
};

client_interface_get_last_error_t client_interface_get_last_error;
client_interface_create_t client_interface_create;
client_interface_destroy_t client_interface_destroy;
client_interface_mainloop_t client_interface_mainloop;
client_interface_log_t client_interface_log;
client_interface_user_file_path_prompt_t client_interface_user_file_path_prompt;

#ifdef __cplusplus
}
#endif
