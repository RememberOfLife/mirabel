#pragma once

#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

namespace Games {

    class TicTacToe : public BaseGameVariant {
        public:
            TicTacToe();
            ~TicTacToe();

            surena::Game* new_game() override;
            void draw_options() override;
            void draw_state_editor(surena::Game* abstract_game) override;
            const char* description() override;

    };

}
