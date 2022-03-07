#include <cstring>

#include "state_control/client.hpp"
#include "state_control/guithread.hpp"
#include "state_control/server.hpp"
#include "state_control/timeout_crash.hpp"

int main(int argc, char *argv[])
{
    //TODO use proper argparsing and offer some more sensible options, e.g. dont use watchdog, etc..
    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        // start server
        StateControl::Server* the_server = new StateControl::Server();
        the_server->loop();
        delete the_server;
        return 0;
    }
    //TODO if launched in client mode, should still start the offline server, which is then paused if later connecting to another server
    StateControl::main_client = new StateControl::Client(); // instantiate the main client
    StateControl::main_client->t_timeout.start();
    StateControl::main_client->t_gui.loop(); // opengl + imgui has to run on the main thread
    StateControl::main_client->~Client(); // destroy the client so everything cleans up nicely
    return 0;
}
