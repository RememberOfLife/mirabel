#include <cstring>

#include "surena/util/semver.h"

#include "control/client.hpp"
#include "control/server.hpp"

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("mirabel version %u.%u.%u\n", Control::client_version.major, Control::client_version.minor, Control::client_version.patch);
        exit(0);
    }
    //TODO use proper argparsing and offer some more sensible options, e.g. dont use watchdog, etc..
    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        // start server
        Control::Server* the_server = new Control::Server();
        the_server->loop();
        delete the_server;
        return 0;
    }
    //TODO if launched in client mode, should still start the offline server, which is then paused if later connecting to another server
    Control::main_client = new Control::Client(); // instantiate the main client
    Control::main_client->loop(); // opengl + imgui has to run on the main thread
    delete Control::main_client; // destroy the client so everything cleans up nicely
    return 0;
}
