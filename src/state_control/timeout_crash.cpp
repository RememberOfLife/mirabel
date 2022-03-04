#include <chrono>
#include <stdio.h>
#include <thread>

#include "meta_gui/meta_gui.hpp"

#include "state_control/controller.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

#include "state_control/timeout_crash.hpp"

namespace StateControl {

    TimeoutCrashThread::TimeoutCrashThread()
    {

    }

    TimeoutCrashThread::~TimeoutCrashThread()
    {

    }
    
    void TimeoutCrashThread::loop()
    {
        main_ctrl->t_gui.inbox.push(event(EVENT_TYPE_HEARTBEAT));
        std::this_thread::sleep_for(std::chrono::milliseconds(initial_sleep));
        bool quit = false;
        while (!quit) {
            bool gui_heartbeat = false;
            for (event e = inbox.pop(); e.type != 0; e = inbox.pop()) {
                // process event e
                // e.g. game updates, load other ctx or game, etc..
                switch (e.type) {
                    case EVENT_TYPE_EXIT: {
                        quit = true;
                        break;
                    } break;
                    case EVENT_TYPE_HEARTBEAT: {
                        gui_heartbeat = true;
                    } break;
                    default: {
                        MetaGui::logf("#W timeout_crash: received unknown event, type: %d\n", e.type);
                    } break;
                }
            }
            if (!gui_heartbeat) {
                fprintf(stderr, "[FATAL] guithread failed to provide heartbeat\n");
                exit(-1); // if the gui has not responded to out heartbeat in timeout ms, quit
            }
            // issue heartbeat to gui
            main_ctrl->t_gui.inbox.push(event(EVENT_TYPE_HEARTBEAT));
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        }
    }
    

    void TimeoutCrashThread::start()
    {
        running_thread = std::thread(&TimeoutCrashThread::loop, this);
    }
    
    void TimeoutCrashThread::join()
    {
        running_thread.join();
    }
    

}
