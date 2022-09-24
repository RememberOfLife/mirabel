#pragma once

#include <cstdint>

#include "mirabel/event_queue.h"
#include "control/lobby.hpp"
#include "control/plugins.hpp"
#include "control/timeout_crash.hpp"
#include "network/network_server.hpp"

namespace Control {

    class Server {

      public:

        TimeoutCrash t_tc;
        TimeoutCrash::timeout_info tc_info;

        Network::NetworkServer* t_network = NULL;
        f_event_queue* network_send_queue = NULL;

        f_event_queue inbox;

        Lobby* lobby = NULL;

        PluginManager plugin_mgr;

        Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
        ~Server();

        void loop();
    };

} // namespace Control
