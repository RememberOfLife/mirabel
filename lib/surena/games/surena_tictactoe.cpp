#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "surena_fast_prng.hpp"

#include "surena_tictactoe.hpp"

namespace surena {

    TicTacToe::TicTacToe():
        state(0b01 << 18) // player one starts
    {}

    uint8_t TicTacToe::player_to_move()
    {
        return static_cast<uint8_t>((state >> 18) & 0b11);
    }

    std::vector<uint64_t> TicTacToe::get_moves()
    {
        std::vector<uint64_t> moves{};
        // if game is over, return empty move list
        if (player_to_move() == 0) {
            return moves;
        }
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                if (get_cell(x, y) == 0) {
                    moves.emplace_back((y<<2)|x);
                }
            }
        }
        return moves;
    }

    void TicTacToe::apply_move(uint64_t move_id)
    {
        // set move as current player
        int x = move_id & 0b11;
        int y = (move_id >> 2) & 0b11;
        int current_player = (state >> 18) & 0b11;
        set_cell(x, y, current_player);
        
        // detect win for current player
        bool win = false;
        for (int i = 0; i < 3; i++) {
            if (get_cell(0, i) == get_cell(1, i) && get_cell(0, i) == get_cell(2, i) && get_cell(0, i) != 0
            || get_cell(i, 0) == get_cell(i, 1) && get_cell(i, 0) == get_cell(i, 2) && get_cell(i, 0) != 0) {
                win = true;
            }
        }
        if ((get_cell(0, 0) == get_cell(1, 1) && get_cell(0, 0) == get_cell(2, 2)
        || get_cell(0, 2) == get_cell(1, 1) && get_cell(0, 2) == get_cell(2, 0)) && get_cell(1, 1) != 0) {
            win = true;
        }
        if (win) {
            // current player has won, set current player to 0 and mark result as current player
            state |= current_player << 20;
            state &= ~(0b11<<18);
            return;
        }
        // detect draw
        bool draw = true;
        for (int i = 0; i < 18; i += 2) {
            if (((state >> i) & 0b11) == 0) {
                draw = false;
                break;
            }
        }
        if (draw) {
            // set current player to 0, result is 0 already
            state &= ~(0b11<<18);
            return;
        }
        // switch player
        int next_player = (current_player == 1) ? 2 : 1;
        state ^= (current_player ^ next_player) << 18;
    }

    uint8_t TicTacToe::get_result()
    {
        return static_cast<uint8_t>((state >> 20) & 0b11);
    }

    uint8_t TicTacToe::perform_playout(uint64_t seed)
    {
        fast_prng rng(seed);
        while (player_to_move() != 0) {
            std::vector<uint64_t> moves_available = get_moves();
            apply_move(moves_available[rng.rand()%moves_available.size()]);
        }
        return get_result();
    }

    surena::PerfectInformationGame* TicTacToe::clone() 
    {
        return new TicTacToe(*this);
    }

    void TicTacToe::copy_from(PerfectInformationGame* target) 
    {
        *this = *static_cast<TicTacToe*>(target);
    }
    
    uint64_t TicTacToe::get_move_id(std::string move_string)
    {
        if (move_string.length() != 2) {
            return UINT64_MAX;
        }
        uint64_t move_id = 0;
        // x
        if (move_string[0] == 'A') {
            move_id |= 0b00;
        } else if (move_string[0] == 'B') {
            move_id |= 0b01;
        } else if (move_string[0] == 'C') {
            move_id |= 0b10;
        } else {
            return UINT64_MAX;
        }
        // y
        if (move_string[1] == '0') {
            move_id |= 0b00<<2;
        } else if (move_string[1] == '1') {
            move_id |= 0b01<<2;
        } else if (move_string[1] == '2') {
            move_id |= 0b10<<2;
        } else {
            return UINT64_MAX;
        }
        return move_id;
    }

    std::string TicTacToe::get_move_string(uint64_t move_id)
    {
        int x = move_id & 0b11;
        int y = (move_id >> 2) & 0b11;
        std::string move_string = "";
        switch (x)
        {
            case (0): {
                move_string += "A";
            } break;
            case (1): {
                move_string += "B";
            } break;
            case (2): {
                move_string += "C";
            } break;
        }
        switch (y)
        {
            case (0): {
                move_string += "0";
            } break;
            case (1): {
                move_string += "1";
            } break;
            case (2): {
                move_string += "2";
            } break;
        }
        return move_string;
    }

    uint8_t TicTacToe::get_cell(int x, int y)
    {
        // shift over the correct 2 bits representing the player at that position
        int v = (state >> (y*6+x*2)) & 0b11;
        return static_cast<uint8_t>(v);
    }

    void TicTacToe::set_cell(int x, int y, uint8_t p)
    {
        uint64_t v = get_cell(x, y);
        int offset = (y*6+x*2);
        // new_state = current_value xor (current_value xor new_value)
        v = v << offset;
        v ^= static_cast<uint64_t>(p) << offset;
        state ^= v;
    }

    void TicTacToe::debug_print()
    {
        for (int y = 2; y >= 0; y--) {
            for (int x = 0; x < 3; x++) {
                switch (get_cell(x, y)) {
                    case (1): {
                        printf("X");
                    } break;
                    case (2): {
                        printf("O");
                    } break;
                    default: {
                        printf(".");
                    } break;
                }
            }
            printf("\n");
        }
    }

}
