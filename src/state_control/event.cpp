#include <cstddef>

#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "state_control/event.hpp"

namespace StateControl {

    event::event(uint32_t type):
        type(type)
    {}

    event::event(const event& e):
        type(e.type)
    {
        switch (e.type) {
            case EVENT_TYPE_GAME_LOAD: {
                game.game = e.game.game;
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                move.code = e.move.code;
            } break;
            case EVENT_TYPE_FRONTEND_LOAD: {
                frontend.frontend = e.frontend.frontend;
            } break;
        }
    }

    event event::create_game_event(uint32_t type, surena::Game *game)
    {
        event e = event(type);
        e.game.game = game;
        return e;
    }

    event event::create_move_event(uint32_t type, uint64_t code)
    {
        event e = event(type);
        e.move.code = code;
        return e;
    }

    event event::create_frontend_event(uint32_t type, Frontends::Frontend *frontend)
    {
        event e = event(type);
        e.frontend.frontend = frontend;
        return e;
    }

}
