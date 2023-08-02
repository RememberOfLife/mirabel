#include <cstdlib>
#include <cstring>

#include "rosalia/semver.h"

#include "control/client.hpp"
#include "control/server.hpp"
#include "generated/git_commit_hash.h"

int main(int argc, char** argv)
{
    //TODO replace with proper, wait surena main for update
    int w_argc = argc - 1; // remaining arg cnt
    while (w_argc > 0) {
        char* w_arg = argv[argc - (w_argc--)]; // working arg
        char* n_arg = (w_argc > 0) ? argv[argc - w_argc] : NULL; // next arg
        if (strcmp(w_arg, "version") == 0) {
            printf("mirabel client version %u.%u.%u\n", Control::client_version.major, Control::client_version.minor, Control::client_version.patch);
            printf("mirabel server version %u.%u.%u\n", Control::server_version.major, Control::server_version.minor, Control::server_version.patch);
            //TODO api versions?
            printf("git commit hash: %s%s\n", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_DIRTY ? " (dirty)" : "");
            exit(EXIT_SUCCESS);
        } else if (strcmp(w_arg, "server") == 0) {
            //TODO use proper argparsing and offer some more sensible options, e.g. dont use watchdog, etc..
            // start server
            Control::Server* the_server = new Control::Server();
            the_server->loop();
            delete the_server;
            exit(EXIT_SUCCESS);
        } else {
            printf("ignoring unknown argument: \"%s\"\n", w_arg);
        }
    }
    //TODO if launched in client mode, should still start the offline server, which is then paused if later connecting to another server
    Control::main_client = new Control::Client(); // instantiate the main client
    Control::main_client->loop(); // opengl + imgui has to run on the main thread
    delete Control::main_client; // destroy the client so everything cleans up nicely
    return 0;
}

/*

int main(int argc, char** argv)
{
    //TODO fix this up
    int w_argc = argc - 1; // remaining arg cnt
    char* w_arg = argv[1];
    if (w_argc < 1) {
        int str_pad = -15;
        printf("Usage: surena [options]\n");
        printf("Options:\n");
        printf("  %*s%s\n", str_pad, "repl", "Run in repl mode.");
        printf("  %*s%s\n", str_pad, "version", "Display surena version information.");
        exit(EXIT_SUCCESS);
    }
    if (strcmp(w_arg, "version") == 0) {
        printf("surena version %d.%d.%d\n", surena::version.major, surena::version.minor, surena::version.patch);
        printf("apis: game(%lu) engine(%lu) move_history(%lu)\n", SURENA_GAME_API_VERSION, SURENA_ENGINE_API_VERSION, SURENA_MOVE_HISTORY_API_VERSION);
        printf("git commit hash: %s%s\n", GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH, GIT_COMMIT_HASH != NULL && GIT_COMMIT_DIRTY ? " (dirty)" : "");
        exit(EXIT_SUCCESS);
    } else if (strcmp(w_arg, "repl") == 0) {
        //TODO allow for initial input via cli
        return repl();
    }
    printf("unrecognized verb \"%s\"; run \"surena\" for usage\n", w_arg);
    exit(EXIT_FAILURE);
}
*/