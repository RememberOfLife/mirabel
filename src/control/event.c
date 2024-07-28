#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/json.h"
#include "rosalia/serialization.h"

#include "mirabel/alloc.h"
#include "mirabel/log.h"

#include "mirabel/event.h"

/////
// event serialization layouts

const serialization_layout sl_base[] = {
    {SL_TYPE_U32, offsetof(event, type)},
    {SL_TYPE_U32, offsetof(event, workspace_id)},
    {SL_TYPE_U32, offsetof(event, association_id)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_log[] = {
    {SL_TYPE_COMPLEX, offsetof(event_log, base), .ext.layout = sl_base},
    {SL_TYPE_UAUTOP2(event_log, status), offsetof(event_log, status)},
    {SL_TYPE_STRING, offsetof(event_log, str)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_neta_open[] = {
    {SL_TYPE_COMPLEX, offsetof(event_neta_open, base), .ext.layout = sl_base},
    {SL_TYPE_STRING, offsetof(event_neta_open, server_addr)},
    {SL_TYPE_U16, offsetof(event_neta_open, server_port)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_neta_close[] = {
    {SL_TYPE_COMPLEX, offsetof(event_neta_close, base), .ext.layout = sl_base},
    {SL_TYPE_STRING, offsetof(event_neta_close, reason)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_user_auth_info[] = {
    {SL_TYPE_COMPLEX, offsetof(event_user_auth_info, base), .ext.layout = sl_base},
    {SL_TYPE_BOOL, offsetof(event_user_auth_info, is_guest)},
    {SL_TYPE_STRING, offsetof(event_user_auth_info, username)},
    {SL_TYPE_STRING, offsetof(event_user_auth_info, password)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_user_auth_reject[] = {
    {SL_TYPE_COMPLEX, offsetof(event_user_auth_reject, base), .ext.layout = sl_base},
    {SL_TYPE_STRING, offsetof(event_user_auth_reject, reason)},
    {SL_TYPE_STOP},
};

const serialization_layout* sl_event_map[EVENT_TYPE_COUNT] = {
    [EVENT_TYPE_NULL] = sl_base,
    [EVENT_TYPE_DESTROYED] = sl_base,

    [EVENT_TYPE_EXIT] = sl_base,

    [EVENT_TYPE_LOG] = sl_log,

    [EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER] = sl_base,
    [EVENT_TYPE_NETWORK_ADAPTER_INTERNAL_SSL_WRITE] = sl_base,

    [EVENT_TYPE_NETWORK_PROTOCOL_OK] = sl_base,
    [EVENT_TYPE_NETWORK_PROTOCOL_NOK] = sl_base,
    [EVENT_TYPE_NETWORK_PROTOCOL_PING] = sl_base,
    [EVENT_TYPE_NETWORK_PROTOCOL_PONG] = sl_base,

    [EVENT_TYPE_NETWORK_ADAPTER_OPEN] = sl_neta_open,
    [EVENT_TYPE_NETWORK_ADAPTER_CLOSE] = sl_neta_close,
    [EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT] = sl_base,
    [EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT] = sl_base,

    [EVENT_TYPE_USER_AUTH_INFO] = sl_user_auth_info,
    [EVENT_TYPE_USER_AUTH_ACCEPT] = sl_base,
    [EVENT_TYPE_USER_AUTH_REJECT] = sl_user_auth_reject,
};

const serialization_layout sl_event_any[] = {
    {
        SL_TYPE_UNION_INTERNALLY_TAGGED,
        .ext.un.tag_size = sizeof(EVENT_TYPE),
        .ext.un.tag_max = EVENT_TYPE_COUNT,
        .ext.un.tag_map = sl_event_map,
    },
    {SL_TYPE_STOP},
};

/////
// event layout serialization wrapper

void event_write_size(void* buf, size_t v)
{
    raw_stream rs = rs_init(buf);
    rs_w_size(&rs, v);
}

size_t event_read_size(void* buf)
{
    raw_stream rs = rs_init(buf);
    return rs_r_size(&rs);
}

/////
// general purpose event utils

const char* event_type_strings[] = {
    [EVENT_TYPE_NULL] = "NULL",
    [EVENT_TYPE_DESTROYED] = "DESTROYED",

    [EVENT_TYPE_EXIT] = "EXIT",
    [EVENT_TYPE_LOG] = "LOG",

    [EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER] = "EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER",
    [EVENT_TYPE_NETWORK_ADAPTER_INTERNAL_SSL_WRITE] = "EVENT_TYPE_NETWORK_ADAPTER_INTERNAL_SSL_WRITE",

    [EVENT_TYPE_NETWORK_PROTOCOL_OK] = "EVENT_TYPE_NETWORK_PROTOCOL_OK",
    [EVENT_TYPE_NETWORK_PROTOCOL_NOK] = "EVENT_TYPE_NETWORK_PROTOCOL_NOK",
    [EVENT_TYPE_NETWORK_PROTOCOL_PING] = "EVENT_TYPE_NETWORK_PROTOCOL_PING",
    [EVENT_TYPE_NETWORK_PROTOCOL_PONG] = "EVENT_TYPE_NETWORK_PROTOCOL_PONG",

    [EVENT_TYPE_NETWORK_ADAPTER_OPEN] = "EVENT_TYPE_NETWORK_ADAPTER_OPEN",
    [EVENT_TYPE_NETWORK_ADAPTER_CLOSE] = "EVENT_TYPE_NETWORK_ADAPTER_CLOSE",
    [EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT] = "EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT",
    [EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT] = "EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_REJECT",

    [EVENT_TYPE_USER_AUTH_INFO] = "EVENT_TYPE_USER_AUTH_INFO",
    [EVENT_TYPE_USER_AUTH_ACCEPT] = "EVENT_TYPE_USER_AUTH_ACCEPT",
    [EVENT_TYPE_USER_AUTH_REJECT] = "EVENT_TYPE_USER_AUTH_REJECT",
};

const char* event_type_str(EVENT_TYPE type)
{
    if (type < EVENT_TYPE_COUNT) {
        return event_type_strings[type];
    }
    return "unknown";
}

static uint32_t next_association_id = 1;

uint32_t get_new_association_id()
{
    return next_association_id++; //TODO //BUG make this atomic
}

void event_create_zero(event_any* e)
{
    e->base.type = EVENT_TYPE_NULL;
    e->base.workspace_id = EVENT_WORKSPACE_NONE;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type(event_any* e, EVENT_TYPE type)
{
    e->base.type = type;
    e->base.workspace_id = EVENT_WORKSPACE_NONE;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type_assoc(event_any* e, EVENT_TYPE type, uint32_t association_id)
{
    e->base.type = type;
    e->base.workspace_id = EVENT_WORKSPACE_NONE;
    e->base.association_id = association_id;
}

void event_create_type_workspace(event_any* e, EVENT_TYPE type, uint32_t session_id)
{
    e->base.type = type;
    e->base.workspace_id = session_id;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type_workspace_assoc(event_any* e, EVENT_TYPE type, uint32_t session_id, uint32_t association_id)
{
    e->base.type = type;
    e->base.workspace_id = session_id;
    e->base.association_id = association_id;
}

void event_zero(event_any* e)
{
    if (e->base.type != EVENT_TYPE_DESTROYED) {
        event_destroy(e);
    }
    e->base.type = EVENT_TYPE_NULL;
}

size_t event_size(event_any* e)
{
    assert(e->base.type != EVENT_TYPE_DESTROYED);
    return 8 + layout_serializer(GSIT_SIZE, sl_event_any, e, NULL, NULL, NULL);
}

void event_serialize(event_any* e, void* buf)
{
    assert(e->base.type != EVENT_TYPE_DESTROYED);
    event_write_size(buf, event_size(e)); //TODO take some size hint to skip redundant size calculation
    layout_serializer(GSIT_SERIALIZE, sl_event_any, e, NULL, (size_t*)buf + 1, NULL);
}

void event_deserialize(event_any* e, void* buf, void* buf_end)
{
    size_t event_size = (char*)buf_end - (char*)buf;
    if (event_size < 8 || event_read_size(buf) > event_size) {
        event_create_zero(e);
        return;
    }
    size_t ec = layout_serializer(GSIT_DESERIALIZE, sl_event_any, NULL, e, (size_t*)buf + 1, buf_end);
    if (ec == LS_ERR) {
        event_create_zero(e);
        return;
    }
}

void event_copy(event_any* to, event_any* from)
{
    assert(from->base.type != EVENT_TYPE_DESTROYED);
    layout_serializer(GSIT_COPY, sl_event_any, from, to, NULL, NULL);
}

void event_destroy(event_any* e)
{
    assert(e->base.type != EVENT_TYPE_DESTROYED);
    layout_serializer(GSIT_DESTROY, sl_event_any, e, NULL, NULL, NULL);
    e->base.type = EVENT_TYPE_DESTROYED; // catch double destroy errors
}

/////
// event specific constructors

void event_create_log(event_any* e, LOGS status, const char* str, const char* str_end)
{
    if (str_end == NULL) {
        event_create_logf(e, status, "%s", str);
    } else {
        event_create_logf(e, status, "%.*s", str_end - str, str);
    }
}

void event_create_logf(event_any* e, LOGS status, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    event_create_logfv(e, status, fmt, args);
    va_end(args);
}

void event_create_logfv(event_any* e, LOGS status, const char* fmt, va_list args)
{
    event_create_type(e, EVENT_TYPE_LOG);
    e->log.status = status;
    e->log.str = NULL;
    if (fmt != NULL) {
        va_list args_copy;
        va_copy(args_copy, args);
        size_t len = vsnprintf(NULL, 0, fmt, args_copy) + 1;
        va_end(args_copy);
        e->log.str = (char*)mirabel_malloc(len); // OOM not handled here
        vsnprintf(e->log.str, len, fmt, args);
    }
}

void event_create_neta_offline_conn_enter(event_any* e, event_queue* in_queue)
{
    event_create_type(e, EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER);
    e->neta_offline_conn.rx_queue = in_queue;
}

void event_create_neta_open(event_any* e, const char* server_addr, uint16_t server_port)
{
    event_create_type(e, EVENT_TYPE_NETWORK_ADAPTER_OPEN);
    e->neta_open.server_addr = server_addr ? strdup(server_addr) : NULL;
    e->neta_open.server_port = server_port;
}

void event_create_neta_close(event_any* e, const char* reason)
{
    event_create_type(e, EVENT_TYPE_NETWORK_ADAPTER_CLOSE);
    e->neta_close.reason = reason ? strdup(reason) : NULL;
}

void event_create_neta_veri(event_any* e, EVENT_TYPE type, blob thumb, const char* reason)
{
    event_create_type(e, type);
    blob_copy(&e->neta_veri.thumb, &thumb);
    e->neta_veri.reason = reason ? strdup(reason) : NULL;
}

void event_create_user_auth_info(event_any* e, EVENT_TYPE type, bool is_guest, const char* username, const char* password)
{
    event_create_type(e, type);
    e->user_auth_info.is_guest = is_guest;
    e->user_auth_info.username = username ? strdup(username) : NULL;
    e->user_auth_info.password = password ? strdup(password) : NULL;
}

void event_create_user_auth_reject(event_any* e, const char* reason)
{
    event_create_type(e, EVENT_TYPE_NETWORK_ADAPTER_CLOSE);
    e->user_auth_reject.reason = reason ? strdup(reason) : NULL;
}
