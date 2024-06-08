#include <cstdlib>

#include <emscripten.h>

#include "main.hpp"

void setup_platform()
{
    // use global: app_regs
    //TODO register web specific network manager and file-op manager
}

void mainloop()
{
    if (app_info::instance->ui->mainloop()) {
        delete app_info::instance;
        exit(0);
    }
}

int main(int argc, char** argv)
{
    setup_platform();
    app_info::new_app(argc, argv);
    emscripten_set_main_loop(mainloop, 0, true);
    //TODO cleanup not necessary for web?
}
