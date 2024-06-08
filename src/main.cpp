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
    if (argc == 1) {
        instance->ui = new GraphicalImmediateMode();
    } else if (argc == 2) {
        if (strcmp(argv[1], "crl") == 0) {
            instance->ui = new CommandReadLine();
        } else if (strcmp(argv[1], "gim") == 0) {
            instance->ui = new GraphicalImmediateMode();
        } else {
            printf("error: unknown interface mode\n");
            exit(1);
        }
    } else {
        printf("error: too many args\n");
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
