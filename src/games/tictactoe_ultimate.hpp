#pragma once

#include "surena/game.h"

#include "games/game_catalogue.hpp"

namespace Games {

    class TicTacToe_Ultimate : public BaseGameVariant {
        public:
            TicTacToe_Ultimate();
            ~TicTacToe_Ultimate();

            game* new_game(const char* options) override;
            void draw_options() override;
            void draw_state_editor(game* abstract_game) override;
            const char* description() override;

    };

}
