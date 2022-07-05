#pragma once

#include <stdint.h>

#include "mirabel/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct f_event_queue_s {
    char _padding[168];
} f_event_queue;

void f_event_queue_create(f_event_queue* eq);

void f_event_queue_destroy(f_event_queue* eq);

// the queue takes ownership of everything in the event and resets it to type NULL
void f_event_queue_push(f_event_queue* eq, f_event_any* e);

// wait until timeout or event to pop available, non blocking if 0, returns NULL event if none available
void f_event_queue_pop(f_event_queue* eq, f_event_any* e, uint32_t t);

#ifdef __cplusplus
}
#endif
