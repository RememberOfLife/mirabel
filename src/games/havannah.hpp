#pragma once

#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

namespace Games {

    class Havannah : public BaseGameVariant {
        private:
            int size = 8;
            
        public:
            Havannah();
            ~Havannah();

            surena::Game* new_game() override;
            void draw_options() override;
            void draw_state_editor(surena::Game* game) override;
            const char* description() override;

    };

}
