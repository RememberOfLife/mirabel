#include <vector>

#include "surena/engines/randomengine.h"
#include "surena/engine.h"

#include "engines/engine_catalogue.hpp"

namespace Engines {

    std::vector<const engine_methods*> engine_catalogue = {
        &randomengine_ebe,
    };

}
