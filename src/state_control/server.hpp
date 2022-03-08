#pragma once

namespace StateControl {

    class Server {

        public:

            //TimeoutCrashThread t_timeout;
            // networkthread

            Server(); //TODO this should probably take the argument if offline, i.e. no db, auto create single lobby and give all perms
            ~Server();

            void loop();

    };

}
