#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rosalia/config.h"
#include "rosalia/serialization.h"
#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/////
// event types

//TODO types of keepalive checks:
// heartbeat = direct queue object holder alive check
// protocol ping just pings the connected network adapter, do not place in recv_queue
// adapter ping pings into the recv_queue of the connected adapter, i.e. gets to the server main loop where protocol ping gets swallowed by the adapter

typedef enum __attribute__((__packed__)) EVENT_TYPE_E {
    // special events
    EVENT_TYPE_NULL = 0, // ignored event
    EVENT_TYPE_EXIT, // queueholder object stop runners and prepares itself for deconstruction by e.g. join
    EVENT_TYPE_LOG,
    EVENT_TYPE_HEARTBEAT, // purely local event between queueholder and timeoutcrash, networking uses protocol_ping events
    EVENT_TYPE_HEARTBEAT_PREQUIT, // theoretically functions as a HEARTBEAT + HEARTBEAT_SET_TIMEOUT would
    EVENT_TYPE_HEARTBEAT_RESET,
    // normal events
    EVENT_TYPE_GAME_LOAD,
    EVENT_TYPE_GAME_LOAD_METHODS,
    EVENT_TYPE_GAME_UNLOAD,
    EVENT_TYPE_GAME_STATE,
    EVENT_TYPE_GAME_MOVE,
    EVENT_TYPE_GAME_SYNC,
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

    EVENT_TYPE_DYNAMIC, // dynamically typed json event encapsulation

    EVENT_TYPE_COUNT,
    EVENT_TYPE_SIZE_MAX = UINT32_MAX,
} EVENT_TYPE;

static const uint32_t EVENT_CLIENT_NONE = 0; // none / local
static const uint32_t EVENT_CLIENT_SERVER = UINT32_MAX;
static const uint32_t EVENT_LOBBY_NONE = 0;
static const uint32_t EVENT_GAME_SYNC_DEFAULT = 0;

typedef struct event_s {
    EVENT_TYPE type;
    uint32_t client_id;
    uint32_t lobby_id;
    uint32_t _padding;
} event;

typedef union event_any_u event_any;

/////
// general purpose utility functions on events

void event_create_zero(event_any* e);

void event_create_type(event_any* e, EVENT_TYPE type);

void event_create_type_client(event_any* e, EVENT_TYPE type, uint32_t client_id);

void event_zero(event_any* e);

size_t event_size(event_any* e);

void event_serialize(event_any* e, void* buf);

void event_deserialize(event_any* e, void* buf, void* buf_end);

void event_copy(event_any* to, event_any* from);

void event_destroy(event_any* e);

// direct usage

// total size written before an event package MUST include itself
void event_write_size(void* buf, size_t v);
size_t event_read_size(void* buf);

// this does not write, and never assumes, the serialization size which should be present just before the event packet
size_t event_general_serializer(GSIT itype, event_any* in, event_any* out, void* buf, void* buf_end);

/////
// specific event types

typedef struct event_log_s {
    event base;
    char* str;
} event_log;

void event_create_log(event_any* e, const char* str);

//TODO logf

typedef struct event_heartbeat_s {
    event base;
    uint32_t id;
    uint32_t time;
} event_heartbeat;

void event_create_heartbeat(event_any* e, EVENT_TYPE type, uint32_t id, uint32_t time);

typedef struct event_game_load_s {
    event base;
    char* base_name;
    char* variant_name;
    char* impl_name;
    game_init init_info;
} event_game_load;

void event_create_game_load(event_any* e, const char* base_name, const char* variant_name, const char* impl_name, game_init init_info);

typedef struct event_game_load_methods_s {
    event base;
    const game_methods* methods;
    game_init init_info;
} event_game_load_methods;

void event_create_game_load_methods(event_any* e, const game_methods* methods, game_init init_info);

typedef struct event_game_state_s {
    event base;
    char* state;
} event_game_state;

void event_create_game_state(event_any* e, uint32_t client_id, const char* state);

typedef struct event_game_move_s {
    event base;
    player_id player;
    move_data_sync data;
} event_game_move;

void event_create_game_move(event_any* e, player_id player, move_data_sync data);

typedef struct event_game_sync_s {
    event base;
    blob data;
} event_game_sync;

void event_create_game_sync(event_any* e, blob* data);

typedef struct event_frontend_load_s {
    event base;
    void* frontend;
} event_frontend_load;

void event_create_frontend_load(event_any* e, void* frontend);

typedef struct event_ssl_thumbprint_s {
    event base;
    //TODO needs reason string and thumbprint should be managed blob
    size_t thumbprint_len;
    void* thumbprint;
} event_ssl_thumbprint;

void event_create_ssl_thumbprint(event_any* e, EVENT_TYPE type);

//TODO for with thumbprint

typedef struct event_auth_s {
    event base;
    bool is_guest;
    char* username;
    char* password;
} event_auth;

void event_create_auth(event_any* e, EVENT_TYPE type, uint32_t client_id, bool is_guest, const char* username, const char* password);

typedef struct event_auth_fail_s {
    event base;
    char* reason;
} event_auth_fail;

void event_create_auth_fail(event_any* e, uint32_t client_id, const char* reason);

typedef struct event_chat_msg_s {
    event base;
    uint32_t msg_id;
    uint32_t author_client_id;
    uint64_t timestamp;
    char* text;
} event_chat_msg;

void event_create_chat_msg(event_any* e, uint32_t msg_id, uint32_t author_client_id, uint64_t timestamp, const char* text);

typedef struct event_chat_del_s {
    event base;
    uint32_t msg_id;
} event_chat_del;

void event_create_chat_del(event_any* e, uint32_t msg_id);

typedef struct event_dynamic_s {
    event base;
    uint32_t dyn_type;
    uint32_t msg_id;
    cj_ovac* payload;
    blob raw;
} event_dynamic;

// event_any is as large as the largest event
// use for arbitrary events, event arrays and deserialization where type and size are unknown
typedef union event_any_u {
    // list all event types here
    event base;
    event_log log;
    event_heartbeat heartbeat;
    event_game_load game_load;
    event_game_load_methods game_load_methods;
    event_game_state game_state;
    event_game_move game_move;
    event_game_sync game_sync;
    event_frontend_load frontend_load;
    event_ssl_thumbprint ssl_thumbprint;
    event_auth auth;
    event_auth_fail auth_fail;
    event_chat_msg chat_msg;
    event_chat_del chat_del;
    event_dynamic dynamic;
} event_any;

#ifdef __cplusplus
}
#endif
