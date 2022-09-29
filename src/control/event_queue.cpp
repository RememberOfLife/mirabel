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

//TODO this should be a ringbuffer, primary goal is reducing wait times for anyone pushing events into it as far as possible
// make sure to move pushed and popped elements, make this a proper producer-consumer semaphore
struct event_queue_impl {
    std::mutex m;
    std::deque<event_any> q;
    std::condition_variable cv;
};

static_assert(sizeof(event_queue) >= sizeof(event_queue_impl), "event_queue impl size missmatch");

void event_queue_create(event_queue* eq)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    new (eqi) event_queue_impl();
}

void event_queue_destroy(event_queue* eq)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    eqi->~event_queue_impl();
}

void event_queue_push(event_queue* eq, event_any* e)
{
    event_queue_impl* eqi = (event_queue_impl*)eq;
    eqi->m.lock();
    eqi->q.emplace_back(*e);
    eqi->cv.notify_all();
    eqi->m.unlock();
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
