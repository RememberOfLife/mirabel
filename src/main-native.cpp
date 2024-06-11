#include "main.hpp"

void setup_platform()
{
    // use global: app_regs
    //TODO register native specific network managers and file-op manager
}

int main(int argc, char** argv)
{
    setup_platform();
    app_info::new_app(argc, argv);
    bool quit = false;
    while (!quit) {
        quit = app_info::instance->ui->mainloop();
    }
    return 0;
}
