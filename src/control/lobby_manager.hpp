#pragma once

#include <cstddef>
#include <cstdbool>
#include <cstdint>
#include <unordered_map>

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/lobby.hpp"

namespace Control {

    class LobbyManager {
      private:

        std::unordered_map<uint32_t, Lobby*> lobbies;

      public:

        PluginManager* plugin_mgr;
        event_queue* send_queue;

        LobbyManager();
        ~LobbyManager();

        void HandleEvent(event_any e); // the lobby manager does not delete this event
    };

} // namespace Control
