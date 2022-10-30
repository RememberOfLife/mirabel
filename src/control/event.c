#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "surena/util/raw_stream.h"
#include "surena/util/serialization.h"
#include "surena/game.h"

#include "mirabel/config.h"

#include "mirabel/event.h"

/////
// event serialization layouts

const serialization_layout sl_baseonly[] = {
    {SL_TYPE_STOP},
};

const serialization_layout sl_base[] = {
    {SL_TYPE_U32, offsetof(event, type)},
    {SL_TYPE_U32, offsetof(event, client_id)},
    {SL_TYPE_U32, offsetof(event, lobby_id)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_log[] = {
    {SL_TYPE_STRING, offsetof(event_log, str)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_heartbeat[] = {
    {SL_TYPE_U32, offsetof(event_heartbeat, id)},
    {SL_TYPE_U32, offsetof(event_heartbeat, time)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_load[] = {
    {SL_TYPE_STRING, offsetof(event_game_load, base_name)},
    {SL_TYPE_STRING, offsetof(event_game_load, variant_name)},
    {SL_TYPE_STRING, offsetof(event_game_load, impl_name)},
    {SL_TYPE_STRING, offsetof(event_game_load, options)},
    {SL_TYPE_STRING, offsetof(event_game_load, legacy)},
    {SL_TYPE_STRING, offsetof(event_game_load, state)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_state[] = {
    {SL_TYPE_STRING, offsetof(event_game_state, state)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_move[] = {
    {SL_TYPE_U64, offsetof(event_game_move, code)},
    {SL_TYPE_STOP},
};

//BUG currently this just leaks memory
const serialization_layout sl_ssl_thumbprint[] = {
    // {SL_TYPE_SIZE, offsetof(event_ssl_thumbprint, thumbprint_len)},
    // {SL_TYPE_BLOB, offsetof(event_ssl_thumbprint, thumbprint)}, //TODO split into str and thumbprint blob
    {SL_TYPE_STOP},
};

const serialization_layout sl_auth[] = {
    {SL_TYPE_BOOL, offsetof(event_auth, is_guest)},
    {SL_TYPE_STRING, offsetof(event_auth, username)},
    {SL_TYPE_STRING, offsetof(event_auth, password)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_auth_fail[] = {
    {SL_TYPE_STRING, offsetof(event_auth_fail, reason)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_chat_msg[] = {
    {SL_TYPE_U32, offsetof(event_chat_msg, msg_id)},
    {SL_TYPE_U32, offsetof(event_chat_msg, author_client_id)},
    {SL_TYPE_U64, offsetof(event_chat_msg, timestamp)},
    {SL_TYPE_STRING, offsetof(event_chat_msg, text)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_chat_del[] = {
    {SL_TYPE_U32, offsetof(event_chat_del, msg_id)},
    {SL_TYPE_STOP},
};

size_t sl_cjovacptr_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    cj_ovac** ovac_in = (cj_ovac**)obj_in;
    cj_ovac** ovac_out = (cj_ovac**)obj_out;
    switch (itype) {
        case GSIT_NONE: {
            assert(0);
        } break;
        case GSIT_INITZERO: {
            *ovac_in = NULL;
        } break;
        case GSIT_SIZE: {
            return cj_measure(*ovac_in, true, true);
        } break;
        case GSIT_SERIALIZE: {
            return (char*)cj_serialize(buf, *ovac_in, true, true) - (char*)buf;
        } break;
        case GSIT_DESERIALIZE: {
            cj_ovac* povac = cj_deserialize(buf, true);
            if (povac->type == CJ_TYPE_ERROR) {
                cj_ovac_destroy(povac);
                return LS_ERR;
            }
            *ovac_out = povac;
        } break;
        case GSIT_COPY: {
            *ovac_out = cj_ovac_duplicate(*ovac_in);
        } break;
        case GSIT_DESTROY: {
            cj_ovac_destroy(*ovac_in);
        } break;
        case GSIT_COUNT:
        case GSIT_SIZE_MAX: {
            assert(0);
        } break;
    }
    return 0;
}

const serialization_layout sl_dynamic[] = {
    {SL_TYPE_U32, offsetof(event_dynamic, dyn_type)},
    {SL_TYPE_U32, offsetof(event_dynamic, msg_id)},
    {SL_TYPE_CUSTOM, offsetof(event_dynamic, payload), .ext.serializer = sl_cjovacptr_serializer},
    {SL_TYPE_BLOB, offsetof(event_dynamic, raw)},
    {SL_TYPE_STOP},
};

const serialization_layout* event_serialization_layouts[EVENT_TYPE_COUNT] = {
    [EVENT_TYPE_NULL] = sl_baseonly,

    [EVENT_TYPE_LOG] = sl_log,
    [EVENT_TYPE_HEARTBEAT] = sl_heartbeat,
    [EVENT_TYPE_HEARTBEAT_PREQUIT] = sl_heartbeat,
    [EVENT_TYPE_HEARTBEAT_RESET] = sl_heartbeat,
    [EVENT_TYPE_EXIT] = sl_baseonly,

    [EVENT_TYPE_GAME_LOAD] = sl_game_load,
    [EVENT_TYPE_GAME_LOAD_METHODS] = sl_baseonly,
    [EVENT_TYPE_GAME_UNLOAD] = sl_baseonly,
    [EVENT_TYPE_GAME_STATE] = sl_game_state,
    [EVENT_TYPE_GAME_MOVE] = sl_game_move,

    [EVENT_TYPE_FRONTEND_LOAD] = sl_baseonly,
    [EVENT_TYPE_FRONTEND_UNLOAD] = sl_baseonly,

    [EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE] = sl_baseonly,

    [EVENT_TYPE_NETWORK_ADAPTER_LOAD] = sl_baseonly,
    [EVENT_TYPE_NETWORK_ADAPTER_UNLOAD] = sl_baseonly,
    [EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED] = sl_baseonly,
    [EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED] = sl_baseonly,
    [EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT] = sl_ssl_thumbprint,
    [EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL] = sl_ssl_thumbprint,
    [EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED] = sl_baseonly,
    [EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED] = sl_baseonly,

    [EVENT_TYPE_NETWORK_PROTOCOL_OK] = sl_baseonly,
    [EVENT_TYPE_NETWORK_PROTOCOL_NOK] = sl_baseonly,
    [EVENT_TYPE_NETWORK_PROTOCOL_DISCONNECT] = sl_baseonly,
    [EVENT_TYPE_NETWORK_PROTOCOL_PING] = sl_baseonly,
    [EVENT_TYPE_NETWORK_PROTOCOL_PONG] = sl_baseonly,
    [EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET] = sl_baseonly,

    [EVENT_TYPE_USER_AUTHINFO] = sl_auth,
    [EVENT_TYPE_USER_AUTHN] = sl_auth,
    [EVENT_TYPE_USER_AUTHFAIL] = sl_auth_fail,

    [EVENT_TYPE_LOBBY_CHAT_MSG] = sl_chat_msg,
    [EVENT_TYPE_LOBBY_CHAT_DEL] = sl_chat_del,

    [EVENT_TYPE_DYNAMIC] = sl_dynamic,
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

size_t event_general_serializer(GSIT itype, event_any* in, event_any* out, void* buf, void* buf_end)
{
    size_t rsize = 0;
    size_t csize;
    csize = layout_serializer(itype, sl_base, in, out, buf, buf_end);
    if (csize == LS_ERR) {
        return LS_ERR;
    }
    EVENT_TYPE etype = (itype == GSIT_DESERIALIZE ? out->base.type : in->base.type);
    const serialization_layout* layout = event_serialization_layouts[etype];
    assert(layout != NULL);
    rsize += csize;
    if (layout->type != SL_TYPE_STOP) {
        csize = layout_serializer(itype, layout, in, out, (char*)buf + rsize, buf_end);
        if (csize == LS_ERR) {
            return LS_ERR;
        }
        rsize += csize;
    }
    return rsize;
}

/////
// general purpose event utils

void event_create_zero(event_any* e)
{
    e->base.type = EVENT_TYPE_NULL;
    e->base.client_id = EVENT_CLIENT_NONE;
    e->base.lobby_id = EVENT_LOBBY_NONE;
}

void event_create_type(event_any* e, EVENT_TYPE type)
{
    e->base.type = type;
    e->base.client_id = EVENT_CLIENT_NONE;
    e->base.lobby_id = EVENT_LOBBY_NONE;
}

void event_create_type_client(event_any* e, EVENT_TYPE type, uint32_t client_id)
{
    e->base.type = type;
    e->base.client_id = client_id;
    e->base.lobby_id = EVENT_LOBBY_NONE;
}

void event_zero(event_any* e)
{
    event_destroy(e);
    e->base.type = EVENT_TYPE_NULL;
}

size_t event_size(event_any* e)
{
    return 8 + event_general_serializer(GSIT_SIZE, e, NULL, NULL, NULL);
}

void event_serialize(event_any* e, void* buf)
{
    event_write_size(buf, event_size(e)); //TODO take some size hint to skip redundant size calculation
    event_general_serializer(GSIT_SERIALIZE, e, NULL, (size_t*)buf + 1, NULL);
}

void event_deserialize(event_any* e, void* buf, void* buf_end)
{
    size_t event_size = (char*)buf_end - (char*)buf;
    if (event_size < 8 || event_read_size(buf) > event_size) {
        event_create_zero(e);
        return;
    }
    size_t ec = event_general_serializer(GSIT_DESERIALIZE, NULL, e, (size_t*)buf + 1, buf_end);
    if (ec == LS_ERR) {
        event_create_zero(e);
        return;
    }
}

void event_copy(event_any* to, event_any* from)
{
    event_general_serializer(GSIT_COPY, from, to, NULL, NULL);
}

void event_destroy(event_any* e)
{
    event_general_serializer(GSIT_DESTROY, e, NULL, NULL, NULL);
}

/////
// event specific constructors

void event_create_log(event_any* e, const char* log)
{
    event_create_type(e, EVENT_TYPE_LOG);
    e->log.str = log ? strdup(log) : NULL;
}

void event_create_heartbeat(event_any* e, EVENT_TYPE type, uint32_t id, uint32_t time)
{
    event_create_type(e, type);
    e->heartbeat.id = id;
    e->heartbeat.time = time;
}

void event_create_game_load(event_any* e, const char* base_name, const char* variant_name, const char* impl_name, const char* options, const char* legacy, const char* state)
{
    event_create_type(e, EVENT_TYPE_GAME_LOAD);
    e->game_load.base_name = base_name ? strdup(base_name) : NULL;
    e->game_load.variant_name = variant_name ? strdup(variant_name) : NULL;
    e->game_load.impl_name = impl_name ? strdup(impl_name) : NULL;
    e->game_load.options = options ? strdup(options) : NULL;
    e->game_load.legacy = legacy ? strdup(legacy) : NULL;
    e->game_load.state = state ? strdup(state) : NULL;
}

void event_create_game_load_methods(event_any* e, const game_methods* methods, const char* options, const char* legacy, const char* state)
{
    event_create_type(e, EVENT_TYPE_GAME_LOAD_METHODS);
    e->game_load_methods.methods = methods;
    e->game_load_methods.options = options ? strdup(options) : NULL;
    e->game_load_methods.legacy = legacy ? strdup(legacy) : NULL;
    e->game_load_methods.state = state ? strdup(state) : NULL;
}

void event_create_game_state(event_any* e, uint32_t client_id, const char* state)
{
    event_create_type_client(e, EVENT_TYPE_GAME_STATE, client_id);
    e->game_state.state = state ? strdup(state) : NULL;
}

void event_create_game_move(event_any* e, move_code code)
{
    event_create_type(e, EVENT_TYPE_GAME_MOVE);
    e->game_move.code = code;
}

void event_create_frontend_load(event_any* e, void* frontend)
{
    event_create_type(e, EVENT_TYPE_FRONTEND_LOAD);
    e->frontend_load.frontend = frontend;
}

void event_create_ssl_thumbprint(event_any* e, EVENT_TYPE type)
{
    event_create_type(e, type);
    e->ssl_thumbprint.thumbprint_len = 0;
    e->ssl_thumbprint.thumbprint = NULL;
}

void event_create_auth(event_any* e, EVENT_TYPE type, uint32_t client_id, bool is_guest, const char* username, const char* password)
{
    event_create_type_client(e, type, client_id);
    e->auth.is_guest = is_guest;
    e->auth.username = username ? strdup(username) : NULL;
    e->auth.password = password ? strdup(password) : NULL;
}

void event_create_auth_fail(event_any* e, uint32_t client_id, const char* reason)
{
    event_create_type_client(e, EVENT_TYPE_USER_AUTHFAIL, client_id);
    e->auth_fail.reason = reason ? strdup(reason) : NULL;
}

void event_create_chat_msg(event_any* e, uint32_t msg_id, uint32_t author_client_id, uint64_t timestamp, const char* text)
{
    event_create_type(e, EVENT_TYPE_LOBBY_CHAT_MSG);
    e->chat_msg.msg_id = msg_id;
    e->chat_msg.author_client_id = author_client_id;
    e->chat_msg.timestamp = timestamp;
    e->chat_msg.text = text ? strdup(text) : NULL;
}

void event_create_chat_del(event_any* e, uint32_t msg_id)
{
    event_create_type(e, EVENT_TYPE_LOBBY_CHAT_DEL);
    e->chat_del.msg_id = msg_id;
}
