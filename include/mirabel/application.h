#pragma once

#include "rosalia/semver.h"

#include "mirabel/client_interface.h"
#include "mirabel/method_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_s {
    method_registry* registry;
    // server* aserver;
    // client* aclient;
    client_interface* interface;
} app;

extern app appi; // global singleton instance

void app_create();

void app_destroy();

void app_args(int argc, char** argv);

bool app_mainloop(); // returns true to shutdown

#ifdef __cplusplus
}
#endif
