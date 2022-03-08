#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/timeout_crash.hpp"

#include "state_control/client.hpp"

namespace StateControl {

    Client* main_client = NULL;

    Client::Client()
    {
        t_timeout.start();
    }

    Client::~Client()
    {
        t_timeout.inbox.push(EVENT_TYPE_EXIT);
        t_timeout.join();
        // t_gui gets auto destructed
    }

}
