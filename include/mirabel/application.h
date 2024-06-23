#pragma once

#include <stdbool.h>

#include "rosalia/argparse.h"

#include "mirabel/client_interface.h"
#include "mirabel/client.h"
#include "mirabel/method_registry.h"
#include "mirabel/server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_s {
    rosa_argpv args;
    method_registry registry;
    //TODO do these really need to be pointers?:
    server* aserver;
    client* aclient;
    client_interface* interface; //TODO are we supporting multiple interfaces or replacing the interface after creation?
} app;

extern app appi; // global singleton instance

void app_create();

void app_destroy();

void app_args(int argc, char** argv);

bool app_mainloop(); // returns true to shutdown

#ifdef __cplusplus
}
#endif
