#pragma once

#include <stdint.h>

#include "rosalia/semver.h"

#include "mirabel/event_queue.h"
#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_APP_INTERFACE_API_VERSION = 1;

typedef uint32_t error_code;
static const uint32_t app_interface_ERR_OK = 0;

typedef struct app_interface_s app_interface;

typedef const char* app_interface_get_last_error_t(app_interface* self);

//TODO supply with context ptr to use, or put that in the running interface object
// if this fails, call destroy
typedef error_code app_interface_create_t(app_interface* self);

typedef void app_interface_destroy_t(app_interface* self);

// return true to request shutdown, the interface will then only be called again once, for destruction, at some arbitrary time
typedef bool app_interface_mainloop_t(app_interface* self);

typedef void app_interface_log_t(app_interface* self, LOGS status, const char* str, const char* str_end);

// if suggested_save_name is NULL this is a load prompt, otherwise save prompt
typedef const char* app_interface_user_file_path_prompt_t(app_interface* self, const char* suggested_save_name);

typedef struct app_interface_methods_s {

    const char* name;
    const semver version;

    app_interface_get_last_error_t* get_last_error;
    app_interface_create_t* create;
    app_interface_destroy_t* destroy;
    app_interface_mainloop_t* mainloop;
    app_interface_log_t* log;
    app_interface_user_file_path_prompt_t* user_file_path_prompt;

} app_interface_methods;

struct app_interface_s {
    const app_interface_methods* methods;
    void* data; // owned by the methods
    event_queue inbox; // incoming "information edges" from the client
};

app_interface_get_last_error_t app_interface_get_last_error;
app_interface_create_t app_interface_create;
app_interface_destroy_t app_interface_destroy;
app_interface_mainloop_t app_interface_mainloop;
app_interface_log_t app_interface_log;
app_interface_user_file_path_prompt_t app_interface_user_file_path_prompt;

#ifdef __cplusplus
}
#endif
