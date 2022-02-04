#include <thread>

#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"

#include "state_control/enginethread.hpp"

namespace StateControl {

    EngineThread::EngineThread()
    {

    }

    EngineThread::~EngineThread()
    {
        
    }

    void EngineThread::loop()
    {

    }

    void EngineThread::start()
    {
        running_thread = std::thread(&EngineThread::loop, this);
    }

    void EngineThread::stop()
    {
        inbox.push(event(EVENT_TYPE_EXIT));
    }
    
    void EngineThread::join()
    {
        running_thread.join();
    }

}
