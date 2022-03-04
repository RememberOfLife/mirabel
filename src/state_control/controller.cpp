#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/timeout_crash.hpp"

#include "state_control/controller.hpp"

namespace StateControl {

    Controller* main_ctrl = new Controller();

    Controller::Controller()
    {}

    Controller::~Controller()
    {
        t_timeout.inbox.push(EVENT_TYPE_EXIT);
        t_timeout.join();
    }

}
