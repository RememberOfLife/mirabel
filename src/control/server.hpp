#pragma once

#include <cstdint>

#include "control/event_queue.hpp"
#include "control/lobby.hpp"
#include "network/network_server.hpp"

namespace Control {

    class Server {

        public:

            //TimeoutCrashThread t_timeout;
            Network::NetworkServer* t_network = NULL;
            event_queue* network_send_queue = NULL;

            event_queue inbox;

            Lobby* lobby = NULL;

            Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
            ~Server();

            void loop();

    };

}
