#include <thread>

#include "state_control/event_queue.hpp"

#include "network/network_adapter.hpp"

namespace Network {

    class NetworkClient : public NetworkAdapter {
        private:
            std::thread runner;

            //TODO the actual network socket

        public:
            NetworkClient();
            ~NetworkClient();

            void loop() override;
            void start() override;
            void join() override;
    };

}
