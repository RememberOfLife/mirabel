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
    {}

    TimeoutCrashThread::~TimeoutCrashThread()
    {}
    
    void TimeoutCrashThread::loop()
    {
        main_ctrl->t_gui.inbox.push(event(EVENT_TYPE_HEARTBEAT));
        bool quit = false;
        const int interval_budget_ms = (1000)/30;
        bool gui_heartbeat = false;
        int gui_last_heartbeat_ms = -initial_delay;

        while (!quit) {
            // check inbox approximately each 30fps frame, this provides responsive exit bahviour
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_budget_ms));

            for (event e = inbox.pop(); e.type != 0; e = inbox.pop()) {
                // process event e
                // e.g. game updates, load other ctx or game, etc..
                switch (e.type) {
                    case EVENT_TYPE_EXIT: {
                        quit = true;
                        MetaGui::log("#I timeout_crash: exit\n");
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

            if (gui_last_heartbeat_ms > timeout_ms) {
                if (gui_heartbeat) {
                    gui_last_heartbeat_ms -= timeout_ms;
                    gui_heartbeat = false;
                    // re-issue heartbeat to gui
                    main_ctrl->t_gui.inbox.push(event(EVENT_TYPE_HEARTBEAT));
                } else {
                    // if the gui has not responded to out heartbeat in timeout ms, quit
                    fprintf(stderr, "[FATAL] guithread failed to provide heartbeat\n");
                    exit(-1);
                }
            }

            gui_last_heartbeat_ms += interval_budget_ms;
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
