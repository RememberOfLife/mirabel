#pragma once

namespace StateControl {

    class Server {

        public:

            //TimeoutCrashThread t_timeout;
            // networkthread

            Server();
            ~Server();

            void loop();

    };

}
