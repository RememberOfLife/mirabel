#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rosalia/json.h"
#include "rosalia/serialization.h"

#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/////
// fwd

typedef struct event_queue_s event_queue;

/////
// event types

typedef enum EVENT_TYPE_E {
    // special events
    EVENT_TYPE_NULL = 0, // ignored event
    EVENT_TYPE_EXIT,
    EVENT_TYPE_LOG,

    EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER,
    EVENT_TYPE_NETWORK_ADAPTER_INTERNAL_SSL_WRITE,

    EVENT_TYPE_NETWORK_PROTOCOL_OK,
    EVENT_TYPE_NETWORK_PROTOCOL_NOK,
    EVENT_TYPE_NETWORK_PROTOCOL_PING,
    EVENT_TYPE_NETWORK_PROTOCOL_PONG,

    EVENT_TYPE_NETWORK_ADAPTER_OPEN,
    EVENT_TYPE_NETWORK_ADAPTER_CLOSE,
    EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT,
    EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT,

    EVENT_TYPE_USER_AUTH_INFO,
    EVENT_TYPE_USER_AUTH_ACCEPT,
    EVENT_TYPE_USER_AUTH_REJECT,

    //TODO session open and close events

    EVENT_TYPE_COUNT,
    EVENT_TYPE_SIZE_MAX = UINT32_MAX,
} EVENT_TYPE;

//TODO typedefs for session and association id?

static const uint32_t EVENT_SESSION_NONE = 0; // none / local
static const uint32_t EVENT_SESSION_SPEC = UINT32_MAX; //TODO just reserved for now

static const uint32_t EVENT_ASSOCIATION_NONE = 0;
static const uint32_t EVENT_ASSOCIATION_SPEC = UINT32_MAX; //TODO just reserved for now

typedef struct event_s {
    EVENT_TYPE type;
    uint32_t connection_id;
    uint32_t session_id;
    uint32_t association_id;
} event;

typedef union event_any_u event_any;

/////
// general purpose event utils

const char* event_type_str(EVENT_TYPE type);

uint32_t get_new_association_id();

void event_create_zero(event_any* e);

void event_create_type(event_any* e, EVENT_TYPE type);

void event_create_type_assoc(event_any* e, EVENT_TYPE type, uint32_t association_id);

void event_create_type_session(event_any* e, EVENT_TYPE type, uint32_t session_id);

void event_create_type_session_assoc(event_any* e, EVENT_TYPE type, uint32_t session_id, uint32_t association_id);

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

/////
// specific event types

typedef struct event_log_s {
    event base;
    LOGS status;
    char* str;
} event_log;

//TODO allow for logging without str/fmt?
void event_create_log(event_any* e, LOGS status, const char* str, const char* str_end);
void event_create_logf(event_any* e, LOGS status, const char* fmt, ...);
void event_create_logfv(event_any* e, LOGS status, const char* fmt, va_list args);

typedef struct event_neta_offline_conn_s {
    event base;
    event_queue* rx_queue;
} event_neta_offline_conn;

void event_create_neta_offline_conn_enter(event_any* e, event_queue* in_queue);

typedef struct event_neta_open_s {
    event base;
    char* server_addr;
    uint16_t server_port;
} event_neta_open;

void event_create_neta_open(event_any* e, const char* server_addr, uint16_t server_port);

typedef struct event_neta_close_s {
    event base;
    char* reason;
} event_neta_close;

void event_create_neta_close(event_any* e, const char* reason);

typedef struct event_neta_veri_s {
    event base;
    blob thumb;
    char* reason;
} event_neta_veri;

void event_create_neta_veri(event_any* e, EVENT_TYPE type, blob thumb, const char* reason);

typedef struct event_user_auth_info_s {
    event base;
    bool is_guest;
    char* username;
    char* password;
} event_user_auth_info;

void event_create_user_auth_info(event_any* e, EVENT_TYPE type, bool is_guest, const char* username, const char* password);

typedef struct event_user_auth_reject_s {
    event base;
    char* reason;
} event_user_auth_reject;

void event_create_user_auth_reject(event_any* e, const char* reason);

// event_any is as large as the largest event
// use for arbitrary events, event arrays and deserialization where type and size are unknown
typedef union event_any_u {
    // list all event types here
    event base;
    event_log log;
    event_neta_offline_conn neta_offline_conn;
    event_neta_open neta_open;
    event_neta_close neta_close;
    event_neta_veri neta_veri;
    event_user_auth_info user_auth_info;
    event_user_auth_reject user_auth_reject;
} event_any;

#ifdef __cplusplus
}
#endif
