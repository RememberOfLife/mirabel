#pragma once

#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include <cstddef>
#include <cstdbool>
#include <cstdint>

namespace Control {

    class LobbyManager {
      private:

      public:

        event_queue* send_queue;

        LobbyManager();
        ~LobbyManager();
    };

} // namespace Control
