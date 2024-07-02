#pragma once

#include <stdbool.h>

#include "rosalia/argparse.h"
#include "rosalia/semver.h"

#include "mirabel/app_interface.h"
#include "mirabel/client.h"
#include "mirabel/methods_registry.h"
#include "mirabel/server.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const semver app_version;

typedef struct app_s {
    rosa_argpv args;
    methods_registry registry;
    //TODO do these really need to be pointers?:
    server* aserver;
    client* aclient;
    app_interface* interface; //TODO are we supporting multiple interfaces or replacing the interface after creation?
} app;

// global singleton instance
extern app appi;

void app_create();

void app_destroy();

void app_args(int argc, char** argv);

// returns true to shutdown
bool app_mainloop();

#ifdef __cplusplus
}
#endif
