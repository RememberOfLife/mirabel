#include <cstdint>

#include "state_control/event_queue.hpp"

namespace StateControl {

    
        void event_queue::push(event e)
        {
            m.lock();
            q.push_back(event(e));
            m.unlock();
        }

        event event_queue::pop()
        {
            m.lock();
            if (q.size() == 0) {
                m.unlock();
                return event(0, 0, NULL);
            }
            event r = event(q.front());
            q.pop_front();
            m.unlock();
            return r;
        }
        
        event event_queue::peek()
        {
            m.lock();
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
