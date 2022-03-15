#include <cstddef>
#include <cstdlib>

#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "state_control/event.hpp"

namespace StateControl {

    event::event(uint32_t type):
        type(type),
        client_id(0)
    {}

    event::event(uint32_t type, uint32_t client_id):
        type(type),
        client_id(client_id)
    {}

    event::event(const event& e)
    {
        memcpy(this, &e, sizeof(event));
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

    event event::create_engine_event(uint32_t type, surena::Engine *engine)
    {
        event e = event(type);
        e.engine.engine = engine;
        return e;
    }

}
