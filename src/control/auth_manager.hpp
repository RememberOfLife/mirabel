#pragma once

#include <cstddef>
#include <cstdbool>
#include <cstdint>

#include "mirabel/event_queue.h"
#include "mirabel/event.h"

namespace Control {

    class AuthManager {
      private:

      public:

        event_queue* send_queue;

        AuthManager();
        ~AuthManager();
    };

} // namespace Control
