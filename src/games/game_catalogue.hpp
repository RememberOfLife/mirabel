#pragma once

#include <vector>

#include "surena/game.h"

namespace Games {
    
    // every concrete variant has a separate file
    class BaseGameVariant {
        public:
            const char* name;
            BaseGameVariant(const char* name);
            ~BaseGameVariant() = default;
            virtual game* new_game(const char* options = NULL) = 0;
            virtual void draw_options() = 0; // draw options available to configure the game, locked while running
            virtual void draw_state_editor(game* abstract_game) = 0; // draw internal state representation and editing tools
            virtual const char* description() = 0; // name string augmented with important options
    };

    class BaseGame {
        public:
            const char* name;
            std::vector<BaseGameVariant*> variants;
    };

    //TODO maybe this should be an ordered map because string->base_game lookups will be common with the string api, same for variant and impl
    extern std::vector<BaseGame> game_catalogue;

}
