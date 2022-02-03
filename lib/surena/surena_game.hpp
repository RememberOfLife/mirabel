#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace surena {

    class PerfectInformationGame {

        public:

            virtual ~PerfectInformationGame() = default;
            
            //TODO evaluation function
            //TODO game state id (zobrist)

            // returns the playerID to move from this state, 0 if the game is over
            virtual uint8_t player_to_move() = 0;

            // returns an unordered list of available moves
            virtual std::vector<uint64_t> get_moves() = 0;
            //TODO unordered iterator over all available moves

            // performs a move on the gamestate, non reversible
            // moves not in the available moves list cause undefined behaviour
            virtual void apply_move(uint64_t move_id) = 0;

            // returns the winning playerID, 0 if the game is not over yet or is a draw
            virtual uint8_t get_result() = 0;

            // playouts the game by randomly playing legal moves until the game is over, returns get_result
            virtual uint8_t perform_playout(uint64_t seed) = 0;

            virtual PerfectInformationGame* clone() = 0;
            virtual void copy_from(PerfectInformationGame* target) = 0;

            // move id transformation functions
            // uint64_t <-> string
            virtual uint64_t get_move_id(std::string move_string) = 0;
            virtual std::string get_move_string(uint64_t move_id) = 0;

            virtual void debug_print() = 0;

            //TODO general description string?

    };

}
