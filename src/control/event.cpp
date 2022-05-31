#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "surena/game.h"

#include "frontends/frontend_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"

#include "control/event.hpp"

#include <atomic>//REMOVE
namespace Control {

    //REMOVE
    uint32_t get_global_event_id()
    {
        static std::atomic_uint32_t global_event_id(0);
        return global_event_id.fetch_add(1);
    }

    f_event::f_event()
    {
        type = EVENT_TYPE_NULL;
        client_id = 0;
    }

    f_event::f_event(EVENT_TYPE _type)
    {
        type = _type;
        client_id = 0;
    }

    f_event::f_event(EVENT_TYPE _type, uint32_t _client_id)
    {
        type = _type;
        client_id = _client_id;
    }

    // copy construct
    // deep copy everything from other into self
    f_event::f_event(const f_event& other)
    {
        //TODO delete self
        f_event* raw_other = (f_event*)&other;
        event_copy(this, raw_other);
    }

    // move construct
    // take ownership of e.g. pointers from other, set them NULL so their destructor wont get them
    f_event::f_event(f_event&& other)
    {
        //TODO user proper move operation, for now copy and destroy old
        //TODO delete self
        event_copy(this, &other);
        other.zero();
    }

    // copy assign
    // deep delete self owned resources, then deep copy from other into self
    f_event& f_event::operator=(const f_event& other)
    {
        //TODO delete self
        f_event* raw_other = (f_event*)&other;
        if (raw_other != this) {
            event_copy(this, raw_other);
        }
        return *this;
    }

    // move assign
    // deep delete self owned resources, then take ownership of e.g. pointers from other, set them NULL so their destructor wont get them
    f_event& f_event::operator=(f_event&& other)
    {
        //TODO user proper move operation, for now copy and destroy old
        //TODO delete self
        event_copy(this, &other);
        other.zero();
        return *this;
    }
    
    f_event::~f_event()
    {
        //TODO somehow this causes double frees?
        // event_destroy(this);
    }

    void f_event::zero()
    {
        // event_destroy(this);
        type = EVENT_TYPE_NULL;
    }

    f_event_heartbeat::f_event_heartbeat(EVENT_TYPE _type, uint32_t _id, uint32_t _time):
        f_event(_type),
        id(_id),
        time(_time)
    {}

    f_event_game_load::f_event_game_load(const char* _base_name, const char* _variant_name):
        f_event(EVENT_TYPE_GAME_LOAD)
    {
        base_name = _base_name ? strdup(_base_name) : NULL;
        variant_name = _variant_name ? strdup(_variant_name) : NULL;
    }

    f_event_game_state::f_event_game_state(uint32_t _client_id, const char* _state):
        f_event(EVENT_TYPE_GAME_STATE, _client_id)
    {
        state = _state ? strdup(_state) : NULL;
    }

    f_event_game_move::f_event_game_move(move_code _code):
        f_event(EVENT_TYPE_GAME_MOVE),
        code(_code)
    {}

    f_event_frontend_load::f_event_frontend_load(Frontends::Frontend* _frontend):
        f_event(EVENT_TYPE_FRONTEND_LOAD),
        frontend(_frontend)
    {}

    f_event_ssl_thumbprint::f_event_ssl_thumbprint(EVENT_TYPE _type):
        f_event(_type),
        thumbprint_len(0),
        thumbprint(NULL)
    {
        //TODO for with thumbprint
        //REMOVE
    }

    f_event_auth::f_event_auth(EVENT_TYPE _type, uint32_t _client_id, bool _is_guest, const char* _username, const char* _password):
        f_event(_type, _client_id),
        is_guest(_is_guest)
    {
        username = _username ? strdup(_username) : NULL;
        password = _password ? strdup(_password) : NULL;
    }

    f_event_auth_fail::f_event_auth_fail(uint32_t _client_id, const char* _reason):
        f_event(EVENT_TYPE_USER_AUTHFAIL, _client_id)
    {
        reason = _reason ? strdup(_reason) : NULL;
    }

    f_event_chat_msg::f_event_chat_msg(uint32_t _msg_id, uint32_t _author_client_id, uint64_t _timestamp, const char* _text):
        f_event(EVENT_TYPE_LOBBY_CHAT_MSG),
        msg_id(_msg_id),
        author_client_id(_author_client_id),
        timestamp(_timestamp)
    {
        text = _text ? strdup(_text) : NULL;
    }

    f_event_chat_del::f_event_chat_del(uint32_t _msg_id):
        f_event(EVENT_TYPE_LOBBY_CHAT_DEL),
        msg_id(_msg_id)
    {}

}
