#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>

#include "mirabel/event.h"

#include "mirabel/event_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*TODO timed events:
- every queue additionally has an ordered linked list
- every pop, after having offered all immediate events, offers times events from the front of the linked list, IF they are expired
    - use a global timestamp from e.g. rosalia to manage realtime
- new push_timed which takes a delay before this event will be offered
    - but the timed item just stores the time when it will be available (maybe for tracking purposes also store enqueuement time?)
*/

/*TODO multi queue wait:
- need to offer c compatible multi_forward_waiter
- multi_wait function which takes vararg many queues and forwards them to the multi_forward_waiter the user has created beforehand
    - then all the queues have a forward ptr, which effectively forwards all condition variable notifies to the multi_forward_waiter
    - when this function returns you still have to pop all the queues yourself to find out which one was triggered
*/

//TODO ? make sure to move pushed and popped elements, make this a proper producer-consumer semaphore
struct event_queue_impl {
    std::mutex m;
    std::deque<event_any> q;
    std::condition_variable cv;
};

void event_queue_create(event_queue* eq)
{
    assert(sizeof(event_queue) >= sizeof(event_queue_impl)); //TODO remove this and force everyone to use void* for eevent_queues since we can not guarantee their size
    event_queue_impl* eqi = (event_queue_impl*)eq;
    new (eqi) event_queue_impl();
}

void event_queue_destroy(event_queue* eq)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    for (std::deque<event_any>::iterator event_iter = eqi->q.begin(); event_iter != eqi->q.end(); event_iter++) {
        event_destroy(&*event_iter);
    }
    eqi->~event_queue_impl();
}

void event_queue_push(event_queue* eq, event_any* e)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    eqi->m.lock();
    eqi->q.emplace_back(*e);
    eqi->m.unlock();
    eqi->cv.notify_all();
    e->base.type = EVENT_TYPE_NULL;
}

void event_queue_pop(event_queue* eq, event_any* e, uint32_t t)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    std::unique_lock<std::mutex> lock(eqi->m);
    if (eqi->q.size() == 0) {
        if (t > 0) {
            eqi->cv.wait_for(lock, std::chrono::milliseconds(t));
        }
        if (eqi->q.size() == 0) {
            // queue has no available events after timeout, return null event
            e->base.type = EVENT_TYPE_NULL;
            return;
        }
        // go on to output an available event if one has become available
    }
    *e = eqi->q.front();
    eqi->q.pop_front();
}

#ifdef __cplusplus
}
#endif
