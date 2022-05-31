#pragma once

#include <cassert>
#include <cstdint>

#include "SDL_net.h"

#include "surena/game.h"

#include "control/event_base.hpp"
#include "frontends/frontend_catalogue.hpp"

namespace Control {

    //TODO types of keepalive checks:
    // heartbeat = direct queue object holder alive check
    // protocol ping just pings the connected network adapter, do not place in recv_queue
    // adapter ping pings into the recv_queue of the connected adapter, i.e. gets to the server main loop where protocol ping gets swallowed by the adapter

    enum EVENT_TYPE : uint32_t {
        // special events
        EVENT_TYPE_NULL = 0, // ignored event
        EVENT_TYPE_HEARTBEAT, // purely local event between queueholder and timeoutcrash, networking uses protocol_ping events
        EVENT_TYPE_HEARTBEAT_PREQUIT, // theoretically functions as a HEARTBEAT + HEARTBEAT_SET_TIMEOUT would
        EVENT_TYPE_HEARTBEAT_RESET,
        EVENT_TYPE_EXIT, // queueholder object stop runners and prepares itself for deconstruction by e.g. join
        // normal events
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_STATE,
        EVENT_TYPE_GAME_MOVE,
        // client only events
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
        //STUB EVENT_TYPE_ENGINE_LOAD,
        //STUB EVENT_TYPE_ENGINE_UNLOAD,
        // networking events: internal events
        EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE,
        // networking events: adapter events; work with adapter<->main_queue
        EVENT_TYPE_NETWORK_ADAPTER_LOAD,
        EVENT_TYPE_NETWORK_ADAPTER_UNLOAD,
        EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED,
        EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED,
        EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT,
        EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL,
        EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED,
        EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED,
        // networking events: protocol events; work with adapter<->adapter, they should not reach the main queue, and ignored if they do
        EVENT_TYPE_NETWORK_PROTOCOL_OK,
        EVENT_TYPE_NETWORK_PROTOCOL_NOK,
        EVENT_TYPE_NETWORK_PROTOCOL_DISCONNECT,
        EVENT_TYPE_NETWORK_PROTOCOL_PING,
        EVENT_TYPE_NETWORK_PROTOCOL_PONG,
        EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET,
        // user events: deal with the general user information on the connected server
        EVENT_TYPE_USER_AUTHINFO,
        EVENT_TYPE_USER_AUTHN,
        EVENT_TYPE_USER_AUTHFAIL,
        // lobby events: deal with client/server communication
        EVENT_TYPE_LOBBY_CHAT_MSG,
        EVENT_TYPE_LOBBY_CHAT_DEL,

        EVENT_TYPE_COUNT,
    };

    //TODO pack with
    // #pragma pack(1)
    // typedef struct {
    //     data goes here...
    // } __attribute__((aligned(4))) event_netdata;
    // then use:
    // struct event : public event_netdata { ... }



        // static event create_heartbeat_event(uint32_t type, uint32_t id, uint32_t time = 0);
        // static event create_game_event(uint32_t type, const char* base_game, const char* base_game_variant); //TODO opts
        // static event create_move_event(uint32_t type, uint64_t code);
        // static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
        // static event create_user_auth_event(uint32_t type, uint32_t client_id, bool is_guest, const char* username, const char* password);
        // static event create_chat_msg_event(uint32_t type, uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text);
        // static event create_chat_del_event(uint32_t type, uint32_t msg_id);
    
    //TODO set const for client_none i.e. local or server messages

    struct f_event {
        
        EVENT_TYPE type;
        uint32_t client_id;
        uint32_t lobby_id;

        f_event();
        f_event(EVENT_TYPE _type);
        f_event(EVENT_TYPE _type, uint32_t _client_id);
        f_event(const f_event& other); // copy construct
        f_event(f_event&& other); // move construct
        f_event& operator=(const f_event& other); // copy assign
        f_event& operator=(f_event&& other); // move assign
        ~f_event();

        void zero(); // destroy and set type to zero

        //TODO put size+serialize here

        //WARNING don't use this directly, it will probably be the wrong one, use event_catalogue::get_event_serializer instead
        typedef event_plain_serializer<f_event> serializer;

    };

    struct f_event_heartbeat : public f_event {
        uint32_t id;
        uint32_t time;

        f_event_heartbeat(EVENT_TYPE _type, uint32_t _id, uint32_t _time = 0);
    };

    struct f_event_game_load : public f_event {
        char* base_name;
        char* variant_name;

        typedef event_string_serializer<f_event_game_load, 
            &f_event_game_load::base_name,
            &f_event_game_load::variant_name
        > serializer;

        f_event_game_load(const char* _base_name, const char* _name_variant);
    };

    struct f_event_game_state : public f_event {
        char* state;

        typedef event_string_serializer<f_event_game_state, 
            &f_event_game_state::state
        > serializer;

        f_event_game_state(uint32_t _client_id, const char* _state);
    };

    struct f_event_game_move : public f_event {
        move_code code;
        //TODO use move string instead

        f_event_game_move(move_code _code);
    };

    struct f_event_frontend_load : public f_event {
        Frontends::Frontend* frontend;

        f_event_frontend_load(Frontends::Frontend* _frontend);
    };

    struct f_event_ssl_thumbprint : public f_event {
        //TODO needs reason string and thumbprint should be managed blob
        size_t thumbprint_len;
        void* thumbprint;

        f_event_ssl_thumbprint(EVENT_TYPE _type);
    };

    struct f_event_auth : public f_event {
        bool is_guest;
        char* username;
        char* password;

        typedef event_string_serializer<f_event_auth, 
            &f_event_auth::username,
            &f_event_auth::password
        > serializer;

        f_event_auth(EVENT_TYPE _type, uint32_t _client_id, bool _is_guest, const char* _username, const char* _password);
    };

    struct f_event_auth_fail : public f_event {
        char* reason;

        typedef event_string_serializer<f_event_auth_fail, 
            &f_event_auth_fail::reason
        > serializer;

        f_event_auth_fail(uint32_t _client_id, const char* _reason);
    };

    struct f_event_chat_msg : public f_event {
        uint32_t msg_id;
        uint32_t author_client_id;
        uint64_t timestamp;
        char* text;

        typedef event_string_serializer<f_event_chat_msg, 
            &f_event_chat_msg::text
        > serializer;

        f_event_chat_msg(uint32_t _msg_id, uint32_t _author_client_id, uint64_t _timestamp, const char* _text);
    };

    struct f_event_chat_del : public f_event {
        uint32_t msg_id;

        f_event_chat_del(uint32_t _msg_id);
    };

    //#####
    // general event backend here

    template<EVENT_TYPE TYPE, class EVENT>
    struct event_serializer_pair {
        static constexpr EVENT_TYPE event_type = TYPE;
        typedef EVENT event_t;
    };
    //TODO put all these things (excl. catalgue) into seperrate header
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

    };
    typedef event_catalogue<

        event_serializer_pair<EVENT_TYPE_NULL, f_event>,
        event_serializer_pair<EVENT_TYPE_HEARTBEAT, f_event_heartbeat>,
        event_serializer_pair<EVENT_TYPE_HEARTBEAT_PREQUIT, f_event_heartbeat>,
        event_serializer_pair<EVENT_TYPE_HEARTBEAT_RESET, f_event_heartbeat>,
        event_serializer_pair<EVENT_TYPE_EXIT, f_event>,
        
        event_serializer_pair<EVENT_TYPE_GAME_LOAD, f_event_game_load>,
        event_serializer_pair<EVENT_TYPE_GAME_UNLOAD, f_event>,
        event_serializer_pair<EVENT_TYPE_GAME_STATE, f_event_game_state>,
        event_serializer_pair<EVENT_TYPE_GAME_MOVE, f_event_game_move>,
        
        event_serializer_pair<EVENT_TYPE_FRONTEND_LOAD, f_event_frontend_load>,
        event_serializer_pair<EVENT_TYPE_FRONTEND_UNLOAD, f_event>,
        
        event_serializer_pair<EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE, f_event>,
        
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_LOAD, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_UNLOAD, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT, f_event_ssl_thumbprint>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL, f_event_ssl_thumbprint>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED, f_event>,
        
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_OK, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_NOK, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_DISCONNECT, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_PING, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_PONG, f_event>,
        event_serializer_pair<EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET, f_event>,
        
        event_serializer_pair<EVENT_TYPE_USER_AUTHINFO, f_event_auth>,
        event_serializer_pair<EVENT_TYPE_USER_AUTHN, f_event_auth>,
        event_serializer_pair<EVENT_TYPE_USER_AUTHFAIL, f_event_auth_fail>,
        
        event_serializer_pair<EVENT_TYPE_LOBBY_CHAT_MSG, f_event_chat_msg>,
        event_serializer_pair<EVENT_TYPE_LOBBY_CHAT_DEL, f_event_chat_del>

    > EVENT_CATALGOUE;

    // f_any_event is as large as the largest event
    // use for arbitrary events, event arrays and deserialization where type and size are unknown
    struct f_any_event : public f_event {
        private:
            char _padding[100/*EVENT_CATALGOUE::event_max_size() - sizeof(f_event)*/];
        public:
            f_any_event()
            {}
            template<class EVENT>
            f_any_event(EVENT e)
            {
                memcpy(this, &e, sizeof(e));
            }
            template<class EVENT>
            void operator=(EVENT e)
            {
                memcpy(this, &e, sizeof(e));
            }
            template<class EVENT>
            EVENT& cast()
            {
                return *(EVENT*)this;
            }
            //TODO deserialize here
    };

    //TODO get event serializer wrapper

    template<class EVENT>
    static EVENT& event_cast(f_event& e)
    {
        return *(EVENT*)&e;
    }

    static size_t event_size(f_event* e)
    {
        return EVENT_CATALGOUE::get_event_serializer(e->type)->size(e);
    }

    static void event_serialize(f_event* e, void* buf)
    {
        EVENT_CATALGOUE::get_event_serializer(e->type)->serialize(e, &buf);
    }

    static void event_deserialize(f_any_event* e, void* buf, void* buf_end)
    {
        EVENT_TYPE et = *(EVENT_TYPE*)((char*)buf + sizeof(size_t));
        EVENT_CATALGOUE::get_event_serializer(et)->deserialize(e, &buf, buf_end);
    }

    static void event_copy(f_event* to, f_event* from) //TODO need any events here? or otherwise make sure sizes fit
    {
        EVENT_CATALGOUE::get_event_serializer(from->type)->copy(to, from);
    }
    
    static void event_destroy(f_event* e)
    {
        EVENT_CATALGOUE::get_event_serializer(e->type)->destroy(e);
    }

}
