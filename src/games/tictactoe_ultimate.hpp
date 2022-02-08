#pragma once

#include "games/game_catalogue.hpp"

#include "games/tictactoe_ultimate.hpp"

namespace Games {

    class TicTacToe_Ultimate : public BaseGameVariant {
        public:
            TicTacToe_Ultimate();
            ~TicTacToe_Ultimate();

            surena::Game* new_game() override;
            void draw_options() override;
            void draw_state_editor() override;
            const char* description() override;

    };

}
