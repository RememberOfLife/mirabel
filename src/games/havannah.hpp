#pragma once

#include "surena/games/havannah.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

namespace Games {

    class Havannah : public BaseGameVariant {
        private:
            havannah_options opts{
                .size = 8,
            };
            
        public:
            Havannah();
            ~Havannah();

            game* new_game(const char* options) override;
            void draw_options() override;
            void draw_state_editor(game* abstract_game) override;
            const char* description() override;

    };

}
