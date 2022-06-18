#pragma once

#include <vector>

#include "surena/engine.h"

namespace Engines {

    // class Engine {
    //     public:
    //         Engine();
    //         ~Engine() = default;
    //         virtual engine* new_engine() = 0;
    //         virtual void draw_load_options() = 0;
    // };

    extern std::vector<const engine_methods*> engine_catalogue;

}
