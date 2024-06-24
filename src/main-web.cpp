#include <cstdlib>

#include <emscripten.h>

#include "mirabel/application.h"
#include "mirabel/methods_registry.h"

#include "interface/gim/window.h"

void setup_platform()
{
    methods_registry_add(&appi.registry, "client_interface", "gim", &gim_client_interface);
    //TODO register web specific network manager and file-op manager
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
