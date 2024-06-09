#pragma once

#include <cstddef>

#include "interface/interface.hpp"

struct registrations {
    //TODO available network managers
    //TODO the used file manager

    //TODO API
    // this should be c compatible since it leaks into the client and workspace, itself also a part of the client
    // register_component with string
    // get_component by string
    // how to "offer" interface implementations? use methods pattern from games, but for network and interface etc?
};

extern registrations app_regs;

struct app_info {
    static app_info* instance;

    Interface* ui;

    app_info() = delete;
    static void new_app(int argc, char** argv);
    ~app_info();
};
