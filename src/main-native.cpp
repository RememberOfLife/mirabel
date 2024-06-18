#include "mirabel/application.h"

void setup_platform()
{
    //TODO method_registry_add(app.registry, "client_interface", "crl", /*TODO*/);
    //TODO method_registry_add(app.registry, "client_interface", "gim", /*TODO*/);
    //TODO register native specific network managers and file-op manager
}

int main(int argc, char** argv)
{
    app_create();
    setup_platform();
    app_args(argc, argv);
    bool quit = false;
    while (!quit) {
        quit = app_mainloop();
    }
    app_destroy();
    return 0;
}
