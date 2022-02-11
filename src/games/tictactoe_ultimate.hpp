#pragma once

#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

namespace Games {

    class TicTacToe_Ultimate : public BaseGameVariant {
        public:
            TicTacToe_Ultimate();
            ~TicTacToe_Ultimate();

            surena::Game* new_game() override;
            void draw_options() override;
            void draw_state_editor(surena::Game* abstract_game) override;
            const char* description() override;

    };

}
