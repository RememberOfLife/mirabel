#include <vector>

#include "engines/builtin_surena.hpp"

#include "engines/engine_catalogue.hpp"

namespace Engines {

    Engine::Engine(const char* name):
        name(name)
    {}

    std::vector<Engine*> engine_catalogue = {
        new Builtin_Surena(),
    };

}
