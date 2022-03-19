#include <cstddef>
#include <cstdlib>
#include <cstring>

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

    event::event(uint32_t type, uint32_t raw_length, void* raw_data):
        type(type),
        client_id(0),
        raw_length(raw_length),
        raw_data(raw_data)
    {}

    event::event(uint32_t type, uint32_t client_id):
        type(type),
        client_id(client_id),
        raw_data(NULL)
    {}

    event::event(uint32_t type, uint32_t client_id, uint32_t raw_length, void* raw_data):
        type(type),
        client_id(client_id),
        raw_length(raw_length),
        raw_data(raw_data)
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

    event event::create_game_event(uint32_t type, const char* base_game, const char* base_game_variant)
    {
        event e = event(type);
        e.raw_length = strlen(base_game) + strlen(base_game_variant) + 2; // +2 zero terminators
        e.raw_data = malloc(e.raw_length);
        strcpy(static_cast<char*>(e.raw_data), base_game);
        strcpy(static_cast<char*>(e.raw_data)+strlen(base_game)+1, base_game_variant);
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

    event event::create_chat_msg_event(uint32_t type, uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text)
    {
        event e = event(type);
        e.raw_length = sizeof(msg_id) + sizeof(client_id) + sizeof(timestamp) + strlen(text) + 1;
        e.raw_data = malloc(e.raw_length);
        memcpy(static_cast<char*>(e.raw_data), &msg_id, sizeof(uint32_t));
        memcpy(static_cast<char*>(e.raw_data)+sizeof(uint32_t), &client_id, sizeof(uint32_t));
        memcpy(static_cast<char*>(e.raw_data)+sizeof(uint32_t)*2, &timestamp, sizeof(uint64_t));
        strcpy(static_cast<char*>(e.raw_data)+sizeof(uint32_t)*2+sizeof(uint64_t), text);
        return e;
    }

    event event::create_chat_del_event(uint32_t type, uint32_t msg_id)
    {
        event e = event(type);
        e.msg_del.msg_id = msg_id;
        return e;
    }

}
