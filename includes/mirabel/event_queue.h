#pragma once

#include <stdint.h>

#include "mirabel/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct event_queue_s {
    char _padding[168];
} event_queue;

void event_queue_create(event_queue* eq);

void event_queue_destroy(event_queue* eq);

// the queue takes ownership of everything in the event and resets it to type NULL
void event_queue_push(event_queue* eq, event_any* e);

// wait until timeout or event to pop available, non blocking if 0, returns NULL event if none available
void event_queue_pop(event_queue* eq, event_any* e, uint32_t t);

#ifdef __cplusplus
}
#endif
