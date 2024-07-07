#include <cstdlib>

#include <emscripten.h>

#include "mirabel/application.h"
#include "mirabel/methods_registry.h"

#include "interface/gim/window.h"
#include "network/adapters/offline_client.h"
#include "network/adapters/offline_server.h"

void setup_platform()
{
    methods_registry_add(&appi.registry, "app_interface", "gim", &gim_app_interface);

    methods_registry_add(&appi.registry, "network_adapter_client", "offline", &offline_client_app_interface);
    methods_registry_add(&appi.registry, "network_adapter_server", "offline", &offline_server_app_interface);

    //TODO register native specific file-op manager
}

void mainloop()
{
    if (app_mainloop()) {
        emscripten_cancel_main_loop();
        app_destroy();
        exit(0);
    }
}

int main(int argc, char** argv)
{
    app_create();
    setup_platform();
    app_args(argc, argv);
    emscripten_set_main_loop(mainloop, 0, true);
    //TODO cleanup not necessary for web?
}
