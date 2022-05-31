#pragma once

#include "surena/game.h"

#include "games/game_catalogue.hpp"

namespace Games {

    class TicTacToe : public BaseGameVariant {
        public:
            TicTacToe();
            ~TicTacToe();

            game* new_game(const char* options) override;
            void draw_options() override;
            void draw_state_editor(game* abstract_game) override;
            const char* description() override;

    };

}
