#include <cstddef>
#include <cstdlib>

#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "control/event.hpp"

namespace Control {

    event::event():
        type(EVENT_TYPE_NULL),
        client_id(0),
        raw_data(NULL)
    {}

    event::event(uint32_t type):
        type(type),
        client_id(0),
        raw_data(NULL)
    {}

    event::event(uint32_t type, uint32_t client_id):
        type(type),
        client_id(client_id),
        raw_data(NULL)
    {}

    // copy construct
    // deep copy everything from other into self
    event::event(const event& other)
    {
        memcpy(this, &other, sizeof(event));
        if (other.raw_data) {
            this->raw_data = malloc(other.raw_length);
            memcpy(this->raw_data, other.raw_data, other.raw_length);
        }
    }

    // move construct
    // take ownership of e.g. pointers from other, set them NULL so their destructor wont get them
    event::event(event&& other)
    {
        memcpy(this, &other, sizeof(event));
        other.raw_data = NULL;
            other.type = EVENT_TYPE_NULL;
    }

    // copy assign
    // deep delete self owned resources, then deep copy from other into self
    event& event::operator=(const event& other)
    {
        if (&other != this) {
            free(this->raw_data);
            memcpy(this, &other, sizeof(event));
            if (other.raw_data) {
                this->raw_data = malloc(other.raw_length);
                memcpy(this->raw_data, other.raw_data, other.raw_length);
            }
        }
        return *this;
    }

    // move assign
    // deep delete self owned resources, then take ownership of e.g. pointers from other, set them NULL so their destructor wont get them
    event& event::operator=(event&& other)
    {
        if (&other != this) {
            free(this->raw_data);
            memcpy(this, &other, sizeof(event));
            other.raw_data = NULL;
            other.type = EVENT_TYPE_NULL;
        }
        return *this;
    }

    event::~event()
    {
        free(raw_data);
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
