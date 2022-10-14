#pragma once

#include <cstdint>

#include "surena/util/semver.h"

#include "control/auth_manager.hpp"
#include "mirabel/event_queue.h"
#include "control/lobby_manager.hpp"
#include "control/lobby.hpp"
#include "control/plugins.hpp"
#include "control/timeout_crash.hpp"
#include "control/user_manager.hpp"
#include "network/network_server.hpp"

namespace Control {

    extern const semver server_version;

    class Server {

      public:

        TimeoutCrash t_tc;
        TimeoutCrash::timeout_info tc_info;

        Network::NetworkServer* t_network = NULL;
        event_queue* network_send_queue = NULL;

        event_queue inbox;

        Lobby* lobby = NULL;

        PluginManager plugin_mgr;
        LobbyManager lobby_mgr;
        UserManager user_mgr;
        AuthManager auth_mgr;

        Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
        ~Server();

        void loop();
    };

} // namespace Control
