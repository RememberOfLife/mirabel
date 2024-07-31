#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <queue>

#include "rosalia/timestamp.h"

#include "mirabel/event.h"
#include "mirabel/log.h"

#include "mirabel/event_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*TODO multi queue wait:
- need to offer c compatible multi_forward_waiter
- multi_wait function which takes vararg many queues and forwards them to the multi_forward_waiter the user has created beforehand
    - then all the queues have a forward ptr, which effectively forwards all condition variable notifies to the multi_forward_waiter
    - when this function returns you still have to pop all the queues yourself to find out which one was triggered
*/

/*TODO queue multi wait: make it so that multiple people can sensibly wait on one queue?*/

static const uint64_t QUEUE_CANARY_DESTROYED = 0xAAAAAAAAAAAAAAAA;
static const uint64_t QUEUE_CANARY_VALID = 0xa6c33fdedf77e49e; // just some random number

struct delay_event {
    delay_event_stats stats;
    event_any e;

    friend bool operator<(const delay_event& lesser, const delay_event& greater)
    {
        return lesser.stats.release_ts > greater.stats.release_ts;
    }
};

struct event_queue_impl {
    uint64_t canary;
    std::mutex m;
    std::deque<event_any> iq;
    std::priority_queue<delay_event> tq;
    std::condition_variable cv;
};

void event_queue_create(event_queue* eq)
{
    assert(sizeof(event_queue) >= sizeof(event_queue_impl)); //TODO remove this and force everyone to use void* for eevent_queues since we can not guarantee their size
    event_queue_impl* eqi = (event_queue_impl*)eq;
    if (eqi->canary == QUEUE_CANARY_VALID) {
        mirabel_slogf(LOGS_WARN, "suspicious: queue created with valid canary, possible double creation");
    }
    new (eqi) event_queue_impl();
    eqi->canary = QUEUE_CANARY_VALID;
}

void event_queue_destroy(event_queue* eq)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    if (eqi->canary == QUEUE_CANARY_DESTROYED) {
        mirabel_slogf(LOGS_WARN, "suspicious: queue destroyed with destroyed canary, possible double destruction");
    }
    for (std::deque<event_any>::iterator event_iter = eqi->iq.begin(); event_iter != eqi->iq.end(); event_iter++) {
        event_destroy(&*event_iter);
    }
    while (eqi->tq.size() > 0) {
        delay_event de = eqi->tq.top();
        eqi->tq.pop();
        event_destroy(&de.e);
    }
    eqi->~event_queue_impl();
    eqi->canary = QUEUE_CANARY_DESTROYED;
}

void event_queue_push(event_queue* eq, event_any* e)
{
    event_queue_push_delayed(eq, e, 0);
}

void event_queue_push_delayed(event_queue* eq, event_any* e, uint32_t release_delay)
{
    uint64_t ts_now = timestamp_get_ms64();
    event_queue_impl* eqi = (event_queue_impl*)eq;
    assert(eqi->canary == QUEUE_CANARY_VALID);
    eqi->m.lock();
    if (release_delay == 0) {
        eqi->iq.emplace_back(*e);
    } else {
        eqi->tq.emplace((delay_event){
            .stats = (delay_event_stats){
                .enqueue_ts = ts_now,
                .release_ts = ts_now + release_delay,
            },
            .e = *e,
        });
    }
    eqi->m.unlock();
    eqi->cv.notify_one();
    e->base.type = EVENT_TYPE_NULL;
}

bool event_queue_released_event_available(event_queue_impl* eqi, uint64_t release_up_to)
{
    return eqi->iq.size() > 0 || (eqi->tq.size() > 0 && eqi->tq.top().stats.release_ts <= release_up_to);
}

void event_queue_pop(event_queue* eq, event_any* e, uint32_t t_ms)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    assert(eqi->canary == QUEUE_CANARY_VALID);
    std::unique_lock<std::mutex> lock(eqi->m);
    uint64_t now_ts = timestamp_get_ms64();
    uint64_t maximal_timeout_ts = now_ts + t_ms;
    // wait if still any timeout time available and there is no event available right now
    while (!event_queue_released_event_available(eqi, now_ts) && now_ts < maximal_timeout_ts) {
        uint64_t partial_wait_for = maximal_timeout_ts - now_ts; // per default, wait the full rest of the timeout
        if (eqi->tq.size() > 0 && eqi->tq.top().stats.release_ts < maximal_timeout_ts) {
            // if timed item will become available before maximal timeout, wait shorter
            partial_wait_for = eqi->tq.top().stats.release_ts - now_ts;
        }
        eqi->cv.wait_for(lock, std::chrono::milliseconds(partial_wait_for));
        now_ts = timestamp_get_ms64();
    }
    if (!event_queue_released_event_available(eqi, now_ts)) {
        // no available events (after timeout), return null event
        e->base.type = EVENT_TYPE_NULL;
        return;
    }
    // output available event
    if (eqi->iq.size() > 0) {
        *e = eqi->iq.front();
        eqi->iq.pop_front();
    } else {
        *e = eqi->tq.top().e;
        eqi->tq.pop();
    }
}

#ifdef __cplusplus
}
#endif
