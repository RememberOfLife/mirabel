#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "surena/game.h"

#include "frontends/frontend_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "control/event.hpp"

namespace Control {

    const char* user_auth_event::username(void* raw_data)
    {
        return username_size > 0 ? (char*)raw_data : NULL;
    }

    const char* user_auth_event::password(void* raw_data)
    {
        return password_size > 0 ? (char*)raw_data+username_size : NULL;
    }

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

    event event::create_heartbeat_event(uint32_t type, uint32_t id, uint32_t time)
    {
        event e = event(type);
        e.heartbeat.id = id;
        e.heartbeat.time = time;
        return e;
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

    event event::create_user_auth_event(uint32_t type, uint32_t client_id, bool is_guest, const char* username, const char* password)
    {
        event e = event(type, client_id);
        e.user_auth.is_guest = is_guest;
        e.user_auth.username_size = username ? strlen(username) + 1 : 0;
        e.user_auth.password_size = password ? strlen(password) + 1 : 0;
        if ((e.user_auth.username_size + e.user_auth.password_size) > 0) {
            e.raw_length = e.user_auth.username_size + e.user_auth.password_size;
            e.raw_data = malloc(e.raw_length);
        }
        if (username != NULL) {
            strcpy((char*)e.raw_data, username);
        }
        if (password != NULL) {
            strcpy((char*)e.raw_data+e.user_auth.username_size, password);
        }
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
