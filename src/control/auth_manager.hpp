#pragma once

#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include <cstddef>
#include <cstdbool>
#include <cstdint>

namespace Control {

    class AuthManager {
      private:

      public:

        event_queue* send_queue;

        AuthManager();
        ~AuthManager();
    };

} // namespace Control
