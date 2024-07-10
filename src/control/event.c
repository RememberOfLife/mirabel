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
    {SL_TYPE_U32, offsetof(event, session_id)},
    {SL_TYPE_U32, offsetof(event, association_id)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_log[] = {
    {SL_TYPE_COMPLEX, offsetof(event_log, base), .ext.layout = sl_base},
    {SL_TYPE_STRING, offsetof(event_log, str)},
    {SL_TYPE_STOP},
};

const serialization_layout* sl_event_map[EVENT_TYPE_COUNT] = {
    [EVENT_TYPE_NULL] = sl_base,

    [EVENT_TYPE_EXIT] = sl_base,

    [EVENT_TYPE_LOG] = sl_log,
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
    [EVENT_TYPE_EXIT] = "EXIT",
    [EVENT_TYPE_LOG] = "LOG",
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
    e->base.session_id = EVENT_SESSION_NONE;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type(event_any* e, EVENT_TYPE type)
{
    e->base.type = type;
    e->base.session_id = EVENT_SESSION_NONE;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type_session(event_any* e, EVENT_TYPE type, uint32_t session_id)
{
    e->base.type = type;
    e->base.session_id = session_id;
    e->base.association_id = EVENT_ASSOCIATION_NONE;
}

void event_create_type_session_assoc(event_any* e, EVENT_TYPE type, uint32_t session_id, uint32_t association_id)
{
    e->base.type = type;
    e->base.session_id = session_id;
    e->base.association_id = association_id;
}

void event_zero(event_any* e)
{
    event_destroy(e);
    e->base.type = EVENT_TYPE_NULL;
}

size_t event_size(event_any* e)
{
    return 8 + layout_serializer(GSIT_SIZE, sl_event_any, e, NULL, NULL, NULL);
}

void event_serialize(event_any* e, void* buf)
{
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
    layout_serializer(GSIT_COPY, sl_event_any, from, to, NULL, NULL);
}

void event_destroy(event_any* e)
{
    layout_serializer(GSIT_DESTROY, sl_event_any, e, NULL, NULL, NULL);
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
