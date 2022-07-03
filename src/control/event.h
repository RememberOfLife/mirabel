#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surena/game.h"

#ifdef F_EVENT_INTERNAL
#include "control/event_base.hpp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
    //TODO EVENT_TYPE_GAME_SYNC,
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

static const uint32_t F_EVENT_CLIENT_NONE = 0; // none / local
static const uint32_t F_EVENT_CLIENT_SERVER = UINT32_MAX;
static const uint32_t F_EVENT_LOBBY_NONE = 0;

#ifdef F_EVENT_INTERNAL
#define MANAGED_INTERNAL(...) __VA_ARGS__
#define MANAGED_INTERNAL_DEFAULT                             \
typedef Control::event_plain_serializer<f_event> serializer;
#else
#define MANAGED_INTERNAL(...)
#define MANAGED_INTERNAL_DEFAULT
#endif

//TODO pack with
// #pragma pack(1)
// typedef struct {
//     data goes here...
// } __attribute__((aligned(4))) event_netdata;
// then use:
// struct event : public event_netdata { ... }

struct f_event {
    
    EVENT_TYPE type;
    uint32_t client_id;
    uint32_t lobby_id;
    uint32_t _padding;

    //WARNING don't use this directly, it will probably be the wrong one, use event_catalogue::get_event_serializer instead
    MANAGED_INTERNAL_DEFAULT;

};

typedef union f_event_any_u f_event_any;



// general purpose utility functions on events

void f_event_create_zero(f_event_any* e);

void f_event_create_type(f_event_any* e, EVENT_TYPE type);

void f_event_create_type_client(f_event_any* e, EVENT_TYPE type, uint32_t client_id);

void f_event_zero(f_event_any* e);

size_t f_event_size(f_event_any* e);

size_t f_event_raw_size(f_event_any* e);

void f_event_serialize(f_event_any* e, void* buf);

void f_event_deserialize(f_event_any* e, void* buf, void* buf_end);

void f_event_copy(f_event_any* to, f_event_any* from);

void f_event_destroy(f_event_any* e);



// specific event types

typedef struct f_event_heartbeat_s {
    f_event base;
    uint32_t id;
    uint32_t time;
    MANAGED_INTERNAL_DEFAULT;
} f_event_heartbeat;
void f_event_create_heartbeat(f_event_any* e, EVENT_TYPE type, uint32_t id, uint32_t time);

typedef struct f_event_game_load_s {
    f_event base;
    char* base_name;
    char* variant_name;
    char* options;
    MANAGED_INTERNAL(
        typedef Control::event_string_serializer<f_event_game_load_s, 
            &f_event_game_load_s::base_name,
            &f_event_game_load_s::variant_name,
            &f_event_game_load_s::options
        > serializer;
    );
} f_event_game_load;
void f_event_create_game_load(f_event_any* e, const char* base_name, const char* variant_name, const char* options);

typedef struct f_event_game_state_s {
    f_event base;
    char* state;
    MANAGED_INTERNAL(
        typedef Control::event_string_serializer<f_event_game_state_s, 
            &f_event_game_state_s::state
        > serializer;
    );
} f_event_game_state;
void f_event_create_game_state(f_event_any* e, uint32_t client_id, const char* state);

typedef struct f_event_game_move_s {
    f_event base;
    move_code code;
    //TODO use move string instead?
    //TODO player and sync ctr
    MANAGED_INTERNAL_DEFAULT;
}f_event_game_move;
void f_event_create_game_move(f_event_any* e, move_code code);

typedef struct f_event_frontend_load_s {
    f_event base;
    void* frontend;
    MANAGED_INTERNAL_DEFAULT;
}f_event_frontend_load;
void f_event_create_frontend_load(f_event_any* e, void* frontend);

typedef struct f_event_ssl_thumbprint_s {
    f_event base;
    //TODO needs reason string and thumbprint should be managed blob
    size_t thumbprint_len;
    void* thumbprint;
    MANAGED_INTERNAL_DEFAULT;
} f_event_ssl_thumbprint;
void f_event_create_ssl_thumbprint(f_event_any* e, EVENT_TYPE type);
//TODO for with thumbprint

typedef struct f_event_auth_s {
    f_event base;
    bool is_guest;
    char* username;
    char* password;
    MANAGED_INTERNAL(
        typedef Control::event_string_serializer<f_event_auth_s, 
            &f_event_auth_s::username,
            &f_event_auth_s::password
        > serializer;
    );
} f_event_auth;
void f_event_create_auth(f_event_any* e, EVENT_TYPE type, uint32_t client_id, bool is_guest, const char* username, const char* password);

typedef struct f_event_auth_fail_s {
    f_event base;
    char* reason;
    MANAGED_INTERNAL(
        typedef Control::event_string_serializer<f_event_auth_fail_s, 
            &f_event_auth_fail_s::reason
        > serializer;
    );
} f_event_auth_fail;
void f_event_create_auth_fail(f_event_any* e, uint32_t client_id, const char* reason);

typedef struct f_event_chat_msg_s {
    f_event base;
    uint32_t msg_id;
    uint32_t author_client_id;
    uint64_t timestamp;
    char* text;
    MANAGED_INTERNAL(
        typedef Control::event_string_serializer<f_event_chat_msg_s, 
            &f_event_chat_msg_s::text
        > serializer;
    );
} f_event_chat_msg;
void f_event_create_chat_msg(f_event_any* e, uint32_t msg_id, uint32_t author_client_id, uint64_t timestamp, const char* text);

typedef struct f_event_chat_del_s {
    f_event base;
    uint32_t msg_id;
    MANAGED_INTERNAL_DEFAULT;
} f_event_chat_del;
void f_event_create_chat_del(f_event_any* e, uint32_t msg_id);

//#####
// general event backend here

// f_event_any is as large as the largest event
// use for arbitrary events, event arrays and deserialization where type and size are unknown
typedef union f_event_any_u {
    // list all event types here
    f_event base;
    f_event_heartbeat heartbeat;
    f_event_game_load game_load;
    f_event_game_state game_state;
    f_event_game_move game_move;
    f_event_frontend_load frontend_load;
    f_event_ssl_thumbprint ssl_thumbprint;
    f_event_auth auth;
    f_event_auth_fail auth_fail;
    f_event_chat_msg chat_msg;
    f_event_chat_del chat_del;
} f_event_any;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

template<class EVENT>
EVENT& event_cast(f_event_any& e)
{
    return *(EVENT*)&e;
}

#endif

#ifdef F_EVENT_INTERNAL

namespace Control {

    template<EVENT_TYPE TYPE, class EVENT>
    struct event_serializer_pair;

    template<class ...EVENTS>
    struct event_catalogue;

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

    > EVENT_CATALOGUE;

}

#endif
