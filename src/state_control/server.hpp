#pragma once

#include <cstdint>

namespace StateControl {

    enum BP : uint8_t {
        BP_NULL = 0,
        BP_OK,
        BP_NOK,
        BP_PING,
        BP_PONG,
        BP_TEXT,
    };

    class Server {

        public:

            //TimeoutCrashThread t_timeout;
            // networkthread

            Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
            ~Server();

            void loop();

    };

}
