#include "games/chess.hpp"
#include "games/havannah.hpp"
#include "games/tictactoe_ultimate.hpp"
#include "games/tictactoe.hpp"
#include "games/twixt_pp.hpp"

#include "games/game_catalogue.hpp"

namespace Games {

    BaseGameVariant::BaseGameVariant(const char* name):
        name(name)
    {}

    std::vector<BaseGame> game_catalogue = {
        BaseGame{
            "Chess",
            std::vector<BaseGameVariant*>{
                new Chess(),
            }
        },
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
        BaseGame{
            "TwixT",
            std::vector<BaseGameVariant*>{
                new TwixT_PP(),
            }
        },
    };

}
