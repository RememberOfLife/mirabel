#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "control/timeout_crash.hpp"

#include "control/client.hpp"

namespace Control {

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
