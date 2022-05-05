#pragma once

#include <cassert>
#include <cstdint>

#include "SDL_net.h"

#include "surena/engine.hpp"
#include "surena/game.hpp"

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
        EVENT_TYPE_GAME_IMPORT_STATE,
        EVENT_TYPE_GAME_MOVE,
        // client only events
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
        EVENT_TYPE_ENGINE_LOAD,
        EVENT_TYPE_ENGINE_UNLOAD,
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
        EVENT_TYPE_LOBBY_CHAT_MSG, // contains msgId(for removal),client_id(who sent the message),timestamp,text
        EVENT_TYPE_LOBBY_CHAT_DEL,

        EVENT_TYPE_TEST_NONE,
        EVENT_TYPE_TEST_PLAIN,
        EVENT_TYPE_TEST_STRING,

        EVENT_TYPE_COUNT,
    };

    struct heartbeat_event {
        uint32_t id;
        uint32_t time;
    };

    struct move_event {
        uint64_t code;
    };

    struct frontend_event {
        Frontends::Frontend* frontend;
    };

    struct engine_event {
        surena::Engine* engine;
    };

    //TODO important: find some proper way to streamline pointer stuffed events across network and creation
    struct user_auth_event {
        bool is_guest;
        size_t username_size;
        size_t password_size;
        const char* username(void* raw_data);
        const char* password(void* raw_data);
    };

    struct msg_del_event {
        uint32_t msg_id;
    };

    //TODO pack with
    // #pragma pack(1)
    // typedef struct {
    //     data goes here...
    // } __attribute__((aligned(4))) event_netdata;
    // then use:
    // struct event : public event_netdata { ... }

    struct event {
        uint32_t type;
        uint32_t client_id;
        // uint32_t lobby_id; //TODO will go here instead of padding
        // raw data segment, owned by the event
        // when serializing over network this is read and sent together with the packet
        uint32_t raw_length; // len could be stuffed into the first 4 bytes of raw_data, some way to keep two accessors?
        void* raw_data;
        // char _padding[8-sizeof(size_t)]; // when using padding
        union {
            heartbeat_event heartbeat;
            move_event move;
            frontend_event frontend;
            engine_event engine;
            user_auth_event user_auth;
            msg_del_event msg_del;
        };
        event();
        event(uint32_t type);
        event(uint32_t type, uint32_t raw_length, void* raw_data);
        event(uint32_t type, uint32_t client_id);
        event(uint32_t type, uint32_t client_id, uint32_t raw_length, void* raw_data);
        event(const event& other); // copy construct
        event(event&& other); // move construct
        event& operator=(const event& other); // copy assign
        event& operator=(event&& other); // move assign
        ~event();
        static event create_heartbeat_event(uint32_t type, uint32_t id, uint32_t time = 0);
        static event create_game_event(uint32_t type, const char* base_game, const char* base_game_variant);
        static event create_move_event(uint32_t type, uint64_t code);
        static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
        static event create_engine_event(uint32_t type, surena::Engine* frontend);
        static event create_user_auth_event(uint32_t type, uint32_t client_id, bool is_guest, const char* username, const char* password);
        static event create_chat_msg_event(uint32_t type, uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text);
        static event create_chat_del_event(uint32_t type, uint32_t msg_id);
    };



    struct f_event {
        
        EVENT_TYPE type;
        uint32_t client_id;
        uint32_t lobby_id;

        f_event()
        {
            type = EVENT_TYPE_NULL;
        }
        f_event(EVENT_TYPE _type)
        {
            type = _type;
        }

        // event();
        // event(uint32_t type);
        // event(uint32_t type, uint32_t client_id);
        // //TODO zero(); to set type to zero and set serializer
        // event(const event& other); // copy construct
        // event(event&& other); // move construct
        // event& operator=(const event& other); // copy assign
        // event& operator=(event&& other); // move assign
        // ~event();

        //TODO put (de)serialize here

        //WARNING don't use this directly, it will probably be the wrong one, use event_catalogue::get_event_serializer instead
        typedef event_plain_serializer<f_event> serializer;

    };

    struct f_event_test_plain : public f_event {
        uint32_t myown;
        f_event_test_plain(EVENT_TYPE _type, uint32_t _myown):
            f_event(_type),
            myown(_myown)
        {}
    };

    struct f_event_test_string : public f_event {
        uint32_t myother;
        char* mystring;
        char* mystring2;
        typedef event_string_serializer<f_event_test_string, 
            &f_event_test_string::mystring,
            &f_event_test_string::mystring2
        > serializer;
    };

    template<EVENT_TYPE TYPE, class EVENT>
    struct event_serializer_pair {
        static constexpr EVENT_TYPE event_type = TYPE;
        typedef EVENT event_t;
    };
    //TODO put all these things (excl. catalgue) into seperrate header
    template<class ...EVENTS>
    struct event_catalogue {

        // static_assert(sizeof...(EVENTS) == EVENT_TYPE_COUNT, "event catalogue count mismatch"); //TODO put this in again

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
        event_serializer_pair<EVENT_TYPE_TEST_NONE, f_event>,
        event_serializer_pair<EVENT_TYPE_TEST_PLAIN, f_event_test_plain>,
        event_serializer_pair<EVENT_TYPE_TEST_STRING, f_event_test_string>
    > EVENT_CATALGOUE;

    // f_any_event is as large as the largest event
    // use for arbitrary events, event arrays and deserialization where type and size are unknown
    struct f_any_event : public f_event {
        private:
            char _padding[EVENT_CATALGOUE::event_max_size() - sizeof(f_event)];
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

    static void event_copy(f_event* to, f_event* from)
    {
        EVENT_CATALGOUE::get_event_serializer(from->type)->copy(to, from);
    }
    
    static void event_destroy(f_event* e)
    {
        EVENT_CATALGOUE::get_event_serializer(e->type)->destroy(e);
    }

}
