#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "mirabel/application.h"
#include "mirabel/method_registry.h"
#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/////
// internal

void handle_sigterm(int sig)
{
    mirabel_slogf(LOGS_OK, "SIGTERM, initiating graceful shutdown");
    //TODO cleanup ops and notify the mainloop
    exit(0); //TODO not this!
}

/////
// public

app appi;

void app_create()
{
    appi = (app){
        .registry = method_registry_create(),
        .interface = NULL,
    };
    // register SIGTERM handler
    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        mirabel_slogf(LOGS_FATAL, "failed to register SIGTERM handler");
        exit(1); //TODO fail creation gracefully or just exit?
    }
}

void app_destroy()
{
    method_registry_destroy(appi.registry);
}

void app_args(int argc, char** argv)
{
    mirabel_slogf(LOGS_INFO, "ARGS (%i)", argc);
    for (int argi = 0; argi < argc; argi++) {
        mirabel_slogf(LOGS_NORM, "[%i]: %s", argi, argv[argi]);
    }

    // for (int i = 0; i < argc; i++) {
    //     if (strcmp(argv[i], "crl") == 0) {
    //         instance->ui = new CommandReadLine(); //TODO for now unsupported in the web, should be made unavailable via the registration manager
    //         break;
    //     } else if (strcmp(argv[i], "gim") == 0) {
    //         instance->ui = new GraphicalImmediateMode();
    //         break;
    //     }
    // }
    // if (instance->ui == NULL) {
    //     mirabel_slogf(LOGS_FATAL, "no interface set");
    //     exit(1);
    // }
}

bool app_mainloop()
{
    //TODO
    // run server update
    // run client update
    // run interface mainloop
    return true;
}

#ifdef __cplusplus
}
#endif
