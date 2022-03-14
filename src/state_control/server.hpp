#pragma once

#include <cstdint>

#include "network/network_server.hpp"
#include "state_control/guithread.hpp"
#include "state_control/event_queue.hpp"

namespace StateControl {

    class Server {

        public:

            //TimeoutCrashThread t_timeout;
            event_queue inbox;
            Network::NetworkServer* t_network = NULL;
            event_queue* network_send_queue = NULL;

            Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
            ~Server();

            void loop();

    };

}
