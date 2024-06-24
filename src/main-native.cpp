#include "mirabel/application.h"
#include "mirabel/method_registry.h"

#include "interface/crl/cli.h"
#include "interface/gim/window.h"

void setup_platform()
{
    methods_registry_add(&appi.registry, "client_interface", "crl", &cli_client_interface);
    methods_registry_add(&appi.registry, "client_interface", "gim", &gim_client_interface);
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
