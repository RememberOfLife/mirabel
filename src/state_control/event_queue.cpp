#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <cstdint>

#include "meta_gui/meta_gui.hpp"

#include "state_control/event_queue.hpp"

namespace StateControl {

    
        void event_queue::push(event e)
        {
            m.lock();
            q.push_back(event(e));
            cv.notify_all();
            m.unlock();
        }

        event event_queue::pop(uint32_t timeout_ms)
        {
            std::unique_lock<std::mutex> lock(m);
            if (q.size() == 0) {
                if (timeout_ms > 0) {
                    cv.wait_for(lock, std::chrono::milliseconds(timeout_ms));
                }
                if (q.size() == 0) {
                    // queue has no available events after timeout, return null event
                    return event(EVENT_TYPE_NULL);
                }
                // go on to output an available event if one has become available
            }
            event r = event(q.front());
            q.pop_front();
            return r;
        }
        
        event event_queue::peek()
        {
            m.lock();
            if (q.size() == 0) {
                m.unlock();
                return event(EVENT_TYPE_NULL);
            }
            event r = event(q.front());
            m.unlock();
            return r;
        }
        
        uint32_t event_queue::size()
        {
            m.lock();
            uint32_t r = q.size();
            m.unlock();
            return r;
        }
        
        void event_queue::clear()
        {
            m.lock();
            q.clear();
            m.unlock();
        }

}
