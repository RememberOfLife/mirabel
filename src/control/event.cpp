#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "surena/game.h"

#include "control/event_base.hpp"
#define F_EVENT_INTERNAL
#include "mirabel/event.h"

// event catalogue backend

namespace Control {

    template<EVENT_TYPE TYPE, class EVENT>
    struct event_serializer_pair {
        static constexpr EVENT_TYPE event_type = TYPE;
        typedef EVENT event_t;
    };

    template<class ...EVENTS>
    struct event_catalogue {

        static_assert(sizeof...(EVENTS) == EVENT_TYPE_COUNT, "event catalogue count mismatch");

        template<class X, class FIRST, class ...REST>
        static constexpr size_t event_max_size_impl()
        {
            return (sizeof(typename FIRST::event_t) > event_max_size_impl<X, REST...>())
                ? sizeof(typename FIRST::event_t) : event_max_size_impl<X, REST...>();
        }
        template<class X>
        static constexpr size_t event_max_size_impl()
        {
            return 0;
        }
        static constexpr size_t event_max_size()
        {
            return event_max_size_impl<void, EVENTS...>();
        }

        template<class X, class FIRST, class ...REST>
        static event_serializer* get_event_serializer_impl(EVENT_TYPE type)
        {
            if (FIRST::event_type == type) {
                if (FIRST::event_t::serializer::is_plain) {
                    return event_plain_serializer<typename FIRST::event_t>::instance();
                }
                return FIRST::event_t::serializer::instance();
            }
            return get_event_serializer_impl<X, REST...>(type);
        }
        template<class X>
        static event_serializer* get_event_serializer_impl(EVENT_TYPE type)
        {
            assert(0 && "not a valid type");
            return NULL;
        }
        static event_serializer* get_event_serializer(EVENT_TYPE type)
        {
            return get_event_serializer_impl<void, EVENTS...>(type);
        }

        template<class X, class FIRST, class ...REST>
        static size_t get_event_raw_size_impl(EVENT_TYPE type)
        {
            if (FIRST::event_type == type) {
                return sizeof(typename FIRST::event_t);
            }
            return get_event_raw_size_impl<X, REST...>(type);
        }
        template<class X>
        static size_t get_event_raw_size_impl(EVENT_TYPE type)
        {
            assert(0 && "not a valid type");
            return 0;
        }
        static size_t get_event_raw_size(EVENT_TYPE type)
        {
            return get_event_raw_size_impl<void, EVENTS...>(type);
        }

    };

}

// exposed event api

#ifdef __cplusplus
extern "C" {
#endif
    
// general purpose event utils

void f_event_create_zero(f_event_any* e)
{
    e->base.type = EVENT_TYPE_NULL;
    e->base.client_id = F_EVENT_CLIENT_NONE;
    e->base.lobby_id = F_EVENT_LOBBY_NONE;
}

void f_event_create_type(f_event_any* e, EVENT_TYPE type)
{
    e->base.type = type;
    e->base.client_id = F_EVENT_CLIENT_NONE;
    e->base.lobby_id = F_EVENT_LOBBY_NONE;
}

void f_event_create_type_client(f_event_any* e, EVENT_TYPE type, uint32_t client_id)
{
    e->base.type = type;
    e->base.client_id = client_id;
    e->base.lobby_id = F_EVENT_LOBBY_NONE;
}

void f_event_zero(f_event_any* e)
{
    f_event_destroy(e);
    e->base.type = EVENT_TYPE_NULL;
}

size_t f_event_size(f_event_any* e)
{
    return Control::EVENT_CATALOGUE::get_event_serializer(e->base.type)->size((f_event*)e);
}

size_t f_event_raw_size(f_event_any* e)
{
    return Control::EVENT_CATALOGUE::get_event_raw_size(e->base.type);
}

void f_event_serialize(f_event_any* e, void* buf)
{
    Control::EVENT_CATALOGUE::get_event_serializer(e->base.type)->serialize((f_event*)e, &buf);
}

void f_event_deserialize(f_event_any* e, void* buf, void* buf_end)
{
    EVENT_TYPE et = *(EVENT_TYPE*)((char*)buf + sizeof(size_t));
    Control::EVENT_CATALOGUE::get_event_serializer(et)->deserialize((f_event*)e, &buf, buf_end);
}

void f_event_copy(f_event_any* to, f_event_any* from)
{
    Control::EVENT_CATALOGUE::get_event_serializer(from->base.type)->copy((f_event*)to, (f_event*)from);
}

void f_event_destroy(f_event_any* e)
{
    Control::EVENT_CATALOGUE::get_event_serializer(e->base.type)->destroy((f_event*)e);
}

// event specifics

void f_event_create_log(f_event_any* e, const char* log)
{
    f_event_create_type(e, EVENT_TYPE_LOG);
    e->log.str = log ? strdup(log) : NULL;
}

void f_event_create_heartbeat(f_event_any* e, EVENT_TYPE type, uint32_t id, uint32_t time)
{
    f_event_create_type(e, type);
    e->heartbeat.id = id;
    e->heartbeat.time = time;
}

void f_event_create_game_load(f_event_any* e, const char* base_name, const char* variant_name, const char* impl_name, const char* options)
{
    f_event_create_type(e, EVENT_TYPE_GAME_LOAD);
    e->game_load.base_name = base_name ? strdup(base_name) : NULL;
    e->game_load.variant_name = variant_name ? strdup(variant_name) : NULL;
    e->game_load.impl_name = impl_name ? strdup(impl_name) : NULL;
    e->game_load.options = options ? strdup(options) : NULL;
}

void f_event_create_game_load_methods(f_event_any* e, const game_methods* methods, const char* options)
{
    f_event_create_type(e, EVENT_TYPE_GAME_LOAD_METHODS);
    e->game_load_methods.methods = methods;
    e->game_load_methods.options = options ? strdup(options) : NULL;
}

void f_event_create_game_state(f_event_any* e, uint32_t client_id, const char* state)
{
    f_event_create_type_client(e, EVENT_TYPE_GAME_STATE, client_id);
    e->game_state.state = state ? strdup(state) : NULL;
}

void f_event_create_game_move(f_event_any* e, move_code code)
{
    f_event_create_type(e, EVENT_TYPE_GAME_MOVE);
    e->game_move.code = code;
}

void f_event_create_frontend_load(f_event_any* e, void* frontend)
{
    f_event_create_type(e, EVENT_TYPE_FRONTEND_LOAD);
    e->frontend_load.frontend = frontend;
}

void f_event_create_ssl_thumbprint(f_event_any* e, EVENT_TYPE type)
{
    f_event_create_type(e, type);
    e->ssl_thumbprint.thumbprint_len = 0;
    e->ssl_thumbprint.thumbprint = NULL;
}

void f_event_create_auth(f_event_any* e, EVENT_TYPE type, uint32_t client_id, bool is_guest, const char* username, const char* password)
{
    f_event_create_type_client(e, type, client_id);
    e->auth.is_guest = is_guest;
    e->auth.username = username ? strdup(username) : NULL;
    e->auth.password = password ? strdup(password) : NULL;
}

void f_event_create_auth_fail(f_event_any* e, uint32_t client_id, const char* reason)
{
    f_event_create_type_client(e, EVENT_TYPE_USER_AUTHFAIL, client_id);
    e->auth_fail.reason = reason ? strdup(reason) : NULL;
}

void f_event_create_chat_msg(f_event_any* e, uint32_t msg_id, uint32_t author_client_id, uint64_t timestamp, const char* text)
{
    f_event_create_type(e, EVENT_TYPE_LOBBY_CHAT_MSG);
    e->chat_msg.msg_id = msg_id;
    e->chat_msg.author_client_id = author_client_id;
    e->chat_msg.timestamp = timestamp;
    e->chat_msg.text = text ? strdup(text) : NULL;
}

void f_event_create_chat_del(f_event_any* e, uint32_t msg_id)
{
    f_event_create_type(e, EVENT_TYPE_LOBBY_CHAT_DEL);
    e->chat_del.msg_id = msg_id;
}

#ifdef __cplusplus
}
#endif
