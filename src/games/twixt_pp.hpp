#pragma once

#include "surena/games/twixt_pp.h"
#include "surena/game.h"

#include "games/game_catalogue.hpp"

namespace Games {

    class TwixT_PP : public BaseGameVariant {
        private:
            twixt_pp_options opts{
                .wx = 24,
                .wy = 24,
                .pie_swap = true,
            };
            bool square = true;
            
        public:
            TwixT_PP();
            ~TwixT_PP();

            game* new_game(const char* options) override;
            void draw_options() override;
            void draw_state_editor(game* abstract_game) override;
            const char* description() override;

    };

}
