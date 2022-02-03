#include <thread>

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
    
    void EngineThread::join()
    {
        running_thread.join();
    }

}
