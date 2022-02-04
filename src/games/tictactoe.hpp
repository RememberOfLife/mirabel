#pragma once

#include "games/game_catalogue.hpp"

#include "games/tictactoe.hpp"

namespace Games {

    class TicTacToe : public BaseGameVariant {
        public:
            TicTacToe();
            ~TicTacToe();

            surena::PerfectInformationGame* new_game() override;
            void draw_options() override;
            void draw_state_editor() override;
            const char* description() override;

    };

}
