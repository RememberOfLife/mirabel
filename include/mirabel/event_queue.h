#pragma once

#include <stdint.h>

#include "mirabel/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct event_queue_s {
    char _padding[256]; //TODO after rosalia gains threading functionality from tau utils, make this a full c struct
} event_queue;

typedef struct delay_event_stats_s {
    uint64_t enqueue_ts;
    uint64_t release_ts;
} delay_event_stats;

void event_queue_create(event_queue* eq);

void event_queue_destroy(event_queue* eq);

// the queue takes ownership of everything in the event and resets it to type NULL
void event_queue_push(event_queue* eq, event_any* e);

// the event will only naturally pop after at least release_delay in ms time has passed
// the queue takes ownership of everything in the event and resets it to type NULL
void event_queue_push_delayed(event_queue* eq, event_any* e, uint32_t release_delay);

//TODO pop with delay_event_stats

// wait until timeout in ms or event to pop available, non blocking if 0, returns NULL event if none available
void event_queue_pop(event_queue* eq, event_any* e, uint32_t t_ms);

//TODO force pop even timed events not yet released
// void event_queue_pop_force(event_queue* eq, event_any* e, uint32_t t_ms);

#ifdef __cplusplus
}
#endif
