#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "rosalia/argparse.h"

#include "mirabel/alloc.h"
#include "mirabel/app_interface.h"
#include "mirabel/client.h"
#include "mirabel/debug.h"
#include "mirabel/log.h"
#include "mirabel/methods_registry.h"
#include "mirabel/server.h"
#include "generated/git_commit_hash.h"

#include "mirabel/application.h"

/////
// internal

void handle_sigint(int sig)
{
    mirabel_slogf(LOGS_OK, "SIGINT, immediate shutdown");
    exit(1);
}

void handle_sigterm(int sig)
{
    mirabel_slogf(LOGS_OK, "SIGTERM, initiating graceful shutdown");
    //TODO cleanup ops and notify the mainloop
    exit(0); //TODO not this!
}

/////
// public

const semver app_version = (semver){
    .major = 0,
    .minor = 7,
    .patch = 0,
};

app appi;

bool debug_mode;

void app_create()
{
    struct sigaction sa;
    // register SIGTERM handler
    sa.sa_handler = handle_sigterm;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        mirabel_slogf(LOGS_FATAL, "failed to register SIGTERM handler");
        exit(1); //TODO fail creation gracefully or just exit?
    }
    // register SIGINT handler
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        mirabel_slogf(LOGS_FATAL, "failed to register SIGINT handler");
        exit(1); //TODO fail creation gracefully or just exit?
    }

    methods_registry_create(&appi.registry);

    appi.aserver = NULL;
    appi.aclient = NULL;
    appi.interface = NULL;
}

void app_destroy()
{
    if (appi.interface != NULL) {
        app_interface_destroy(appi.interface);
        mirabel_free(appi.interface);
    }
    if (appi.aclient != NULL) {
        client_destroy(appi.aclient);
        mirabel_free(appi.aclient);
    }
    if (appi.aserver != NULL) {
        server_destroy(appi.aserver);
        mirabel_free(appi.aserver);
    }
    methods_registry_destroy(&appi.registry);
    rosa_argpv_destroy(&appi.args);
}

void app_args(int argc, char** argv)
{
    rosa_argpv* ap = &appi.args;
    rosa_argpv_create(ap, argc, argv);

    if (rosa_argpv_exists(ap, "debug")) {
        debug_mode = true;
    }

    if (debug_mode) {
        mirabel_slogf(LOGS_INFO, "ARGS: (%i)", argc);
        for (int argi = 0; argi < argc; argi++) {
            mirabel_slogf(LOGS_NORM, "[%i] %s", argi, argv[argi]);
        }
        mirabel_slogf(LOGS_INFO, "END");
    }

    // help
    if (rosa_argpv_exists(ap, "help")) {
        mirabel_slogf(LOGS_NORM, "usage: mirabel [key=value]...");
        mirabel_slogf(LOGS_NORM, "");
        mirabel_slogf(LOGS_NORM, "#TODO");
        exit(0); //TODO better exit here
    }

    // version
    if (rosa_argpv_exists(ap, "version")) {
        mirabel_slogf(LOGS_NORM, "mirabel version: %u.%u.%u", app_version.major, app_version.minor, app_version.patch);
        mirabel_slogf(LOGS_NORM, "git commit hash: %s%s", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_DIRTY ? " (dirty)" : "");
        exit(0); //TODO better exit here
    }

    //TODO load methods plugins if specified via args

    bool want_server = rosa_argpv_exists(ap, "server");
    bool no_server = rosa_argpv_val_eq(ap, "server", "0");
    if (no_server) {
        want_server = false;
    }
    bool want_client = rosa_argpv_exists(ap, "client") || !want_server;

    //TODO could also replace this with a "fake" method registration, i.e. a NULL ptr for the server/hosted is registered in main-web.cpp
#ifdef __EMSCRIPTEN__
    if (want_server) {
        mirabel_slogf(LOGS_WARN, "can not create hosting server in web version, using offline server instead");
        want_server = false;
    }
#endif

    appi.aserver = mirabel_malloc(sizeof(server));
    if (want_server) {
        // hosting server
        server_create(appi.aserver, false);
    } else if (!no_server) {
        // offline server
        server_create(appi.aserver, true);
    }

    if (want_client) {
        appi.aclient = mirabel_malloc(sizeof(client));
        client_create(appi.aclient);
    }

    const char* requested_interface = rosa_argpv_val(ap, "interface");
    if (!rosa_argpv_val_eq(ap, "interface", "none") && requested_interface == NULL) {
        if (want_client) {
            // typical mode without args automatically spawns client and gim interface
            requested_interface = "gim";
        } else if (want_server) {
            // if only server and no interface specified, put up crl interface
            requested_interface = "crl";
        }
    }
    if (requested_interface != NULL) {
        const app_interface_methods* found_interface_methods = methods_registry_get(&appi.registry, "app_interface", requested_interface);
        if (found_interface_methods == NULL) {
            mirabel_slogf(LOGS_ERR, "interface \"%s\" not found", requested_interface);
        } else {
            appi.interface = mirabel_malloc(sizeof(app_interface));
            appi.interface->methods = found_interface_methods;
            if (app_interface_create(appi.interface) != APP_INTERFACE_ERR_OK) {
                const char* err_str = app_interface_get_last_error(appi.interface);
                mirabel_slogf(LOGS_ERR, "interface \"%s\" creation failed%s%s", err_str != NULL ? ": " : "", err_str != NULL ? err_str : "");
                app_interface_destroy(appi.interface);
            }
        }
    }
}

bool app_mainloop()
{
    bool shutdown = false;
    shutdown |= server_update(appi.aserver);
    if (appi.aclient != NULL) {
        shutdown |= client_update(appi.aclient);
    }
    if (appi.interface != NULL) {
        shutdown |= app_interface_mainloop(appi.interface);
    }
    if (shutdown) {
        //TODO cleanup if we need to do any in the mainloop
    }
    return shutdown;
}
