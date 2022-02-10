#include "games/havannah.hpp"
#include "games/tictactoe_ultimate.hpp"
#include "games/tictactoe.hpp"

#include "games/game_catalogue.hpp"

namespace Games {

    BaseGameVariant::BaseGameVariant(const char* name):
        name(name)
    {}

    std::vector<BaseGame> game_catalogue = {
        BaseGame{
            "Havannah",
            std::vector<BaseGameVariant*>{
                new Havannah(),
            }
        },
        BaseGame{
            "TicTacToe",
            std::vector<BaseGameVariant*>{
                new TicTacToe(),
                new TicTacToe_Ultimate(),
            }
        },
    };

}
