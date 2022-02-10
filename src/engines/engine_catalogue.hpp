#pragma once

#include <vector>

#include "surena/engine.hpp"

#include "games/game_catalogue.hpp"

namespace Engines {

    class Engine {
        public:
            const char* name;
            Engine(const char* name);
            ~Engine() = default;
            virtual bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) = 0;
            virtual surena::Engine* new_engine() = 0;
            virtual void draw_loader_options() = 0; // draw options available before loading the engine
            virtual void draw_state_options(surena::Engine* engine) = 0; // draw options available as offered by the engine at runtime
            virtual const char* description() = 0; // name string augmented with important options
    };

    extern std::vector<Engine*> engine_catalogue;

}
