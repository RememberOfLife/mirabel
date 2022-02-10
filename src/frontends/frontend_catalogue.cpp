#include "frontends/empty_frontend.hpp"
#include "frontends/havannah.hpp"
#include "frontends/tictactoe_ultimate.hpp"
#include "frontends/tictactoe.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    FrontendWrap::FrontendWrap(const char* name):
        name(name)
    {}

    std::vector<FrontendWrap*> frontend_catalogue = {
        new EmptyFrontend_FEW(),
        new TicTacToe_Ultimate_FEW(),
        new TicTacToe_FEW(),
        new Havannah_FEW(),
    };

}
