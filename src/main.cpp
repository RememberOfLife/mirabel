#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "main.hpp"
#include "interface/crl/cli.hpp"
#include "interface/gim/window.hpp"

registrations app_regs;

app_info* app_info::instance = NULL;

void app_info::new_app(int argc, char** argv)
{
    if (instance != NULL) {
        printf("error: app instance already exists");
        exit(1);
    }

    //TODO check platform registrations for sanity, i.e. non null etc..

    instance = (app_info*)malloc(sizeof(app_info));

    //TODO use proper argparser from rosalia
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "crl") == 0) {
            instance->ui = new CommandReadLine(); //TODO for now unsupported in the web, should be made unavailable via the registration manager
            break;
        } else if (strcmp(argv[i], "gim") == 0) {
            instance->ui = new GraphicalImmediateMode();
            break;
        }
    }
    if (instance->ui == NULL) {
        printf("error: no interface set\n");
        exit(1);
    }
}

app_info::~app_info()
{
    //TODO members
    delete ui;
    // app_info itself
    free(instance);
    instance = NULL;
}
