#include "frontends/chess.hpp"
#include "frontends/empty_frontend.hpp"
#include "frontends/fallback_text.hpp"
#include "frontends/havannah.hpp"
#include "frontends/tictactoe_ultimate.hpp"
#include "frontends/tictactoe.hpp"
#include "frontends/twixt_pp.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Frontends {

    FrontendWrap::FrontendWrap(const char* name):
        name(name)
    {}

    std::vector<FrontendWrap*> frontend_catalogue = {
        new EmptyFrontend_FEW(), // do not move the empty frontend wrapper
        // new FallbackText_FEW(), // also dont move this
        new Chess_FEW(),
        new Havannah_FEW(),
        new TicTacToe_Ultimate_FEW(),
        new TicTacToe_FEW(),
        new TwixT_PP_FEW(),
    };

}
