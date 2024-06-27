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
// event types

typedef enum EVENT_TYPE_E {
    // special events
    EVENT_TYPE_NULL = 0, // ignored event
    EVENT_TYPE_EXIT,
    EVENT_TYPE_LOG,

    EVENT_TYPE_COUNT,
    EVENT_TYPE_SIZE_MAX = UINT32_MAX,
} EVENT_TYPE;

static const uint32_t EVENT_CLIENT_NONE = 0; // none / local
static const uint32_t EVENT_CLIENT_SERVER = UINT32_MAX;

static const uint32_t EVENT_ASSOCIATION_NONE = 0;
static const uint32_t EVENT_ASSOCIATION_SPEC = UINT32_MAX; //TODO just reserved for now

typedef struct event_s {
    EVENT_TYPE type;
    uint32_t client_id;
    uint32_t association_id;
    uint32_t _reserved;
} event;

typedef union event_any_u event_any;

/////
// general purpose event utils

const char* event_type_str(EVENT_TYPE type);

uint32_t get_new_association_id();

void event_create_zero(event_any* e);

void event_create_type(event_any* e, EVENT_TYPE type);

void event_create_type_client(event_any* e, EVENT_TYPE type, uint32_t client_id);

void event_create_type_client_assoc(event_any* e, EVENT_TYPE type, uint32_t client_id, uint32_t association_id);

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

void event_create_log(event_any* e, LOGS status, const char* str, const char* str_end);
void event_create_logf(event_any* e, LOGS status, const char* fmt, ...);
void event_create_logfv(event_any* e, LOGS status, const char* fmt, va_list args);

// event_any is as large as the largest event
// use for arbitrary events, event arrays and deserialization where type and size are unknown
typedef union event_any_u {
    // list all event types here
    event base;
    event_log log;
} event_any;

#ifdef __cplusplus
}
#endif
