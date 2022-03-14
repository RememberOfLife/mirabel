#include <thread>

#include "state_control/event_queue.hpp"

#include "network/network_adapter.hpp"

namespace Network {

    class NetworkServer : public NetworkAdapter {
        private:
            std::thread runner;

            //TODO the actual network socket

        public:
            NetworkServer();
            ~NetworkServer();

            void loop() override;
            void start() override;
            void join() override;
    };

}
