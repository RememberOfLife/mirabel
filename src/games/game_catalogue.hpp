#pragma once

#include <vector>

#include "surena_game.hpp"

namespace Games {
    
    // every concrete variant has a separate file
    class BaseGameVariant {
        public:
            const char* name;
            BaseGameVariant(const char* name);
            ~BaseGameVariant() = default;
            virtual surena::PerfectInformationGame* new_game() = 0;
            virtual void draw_options() = 0; // draw options available to configure the game, locked while running
            virtual void draw_state_editor() = 0; // draw internal state representation and editing tools
            virtual const char* description() = 0; // name string augmented with important options
    };

    class BaseGame {
        public:
            const char* name;
            std::vector<BaseGameVariant*> variants;
    };

    extern std::vector<BaseGame> game_catalogue;

}
