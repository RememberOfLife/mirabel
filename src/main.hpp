#pragma once

#include <cstddef>

#include "interface/interface.hpp"

struct registrations {
    //TODO available network managers
    //TODO the used file manager
};

extern registrations app_regs;

struct app_info {
    static app_info* instance;

    Interface* ui;

    app_info() = delete;
    static void new_app(int argc, char** argv);
    ~app_info();
};
