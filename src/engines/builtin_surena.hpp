#pragma once

#include <cstdint>

#include "surena/engine.hpp"

#include "engines/engine_catalogue.hpp"
#include "games/game_catalogue.hpp"

namespace Engines {

    class Builtin_Surena : public Engine {
        private:
            uint64_t constraints_timeout;
        public:
            Builtin_Surena();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            surena::Engine* new_engine() override;
            void draw_loader_options() override;
            void draw_state_options(surena::Engine* engine) override;
            const char* description() override;
    };

}
