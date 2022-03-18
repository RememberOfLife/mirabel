#pragma once

#include <SDL2/SDL.h>
#include "surena/games/havannah.hpp"
#include "surena/game.hpp"

#include "games/game_catalogue.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    class Havannah : public Frontend {

        public:

            surena::Havannah* game;
            surena::Engine* engine;

            int size;

            // runtime graphics options 
            bool flat_top = false;
            float button_size = 25;
            float padding = 2.5;
            float stone_size_mult = 0.8;
            bool hex_stones = false;
            float connections_width = 0; //TODO same for virtual connections?

            struct sbtn {
                float x;
                float y;
                float r;
                uint8_t ix;
                uint8_t iy;
                bool hovered;
                bool mousedown;
                void update(float mx, float my);
            };

            int mx;
            int my;

            //TODO remove empty buttons and just malloc those actually required
            sbtn board_buttons[361]; // enough buttons for size 10

            Havannah();
            ~Havannah();
            void set_game(surena::Game* new_game) override;
            void set_engine(surena::Engine* new_engine) override;
            void process_event(SDL_Event event) override;
            void update() override;
            void render() override;
            void draw_options() override;

            static void rotate_cords(float& x, float&y, float angle);

    };

    class Havannah_FEW : public FrontendWrap {
        public:
            Havannah_FEW();
            ~Havannah_FEW();
            bool base_game_variant_compatible(Games::BaseGameVariant* base_game_variant) override;
            Frontend* new_frontend() override;
            void draw_options() override;
    };

}
