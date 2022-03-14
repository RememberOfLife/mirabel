#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"

namespace Network {

    class NetworkAdapter {
        protected:
            StateControl::event_queue send_queue;
            StateControl::event_queue* recv_queue;

        public:
            NetworkAdapter();
            ~NetworkAdapter() = default;

            virtual void loop() = 0;
            virtual void start() = 0;
            virtual void join() = 0;
    };

}
