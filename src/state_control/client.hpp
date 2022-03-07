#include "state_control/guithread.hpp"
#include "state_control/timeout_crash.hpp"

namespace StateControl {

    class Client {

        public:

            GuiThread t_gui;
            TimeoutCrashThread t_timeout;
            // networkthread
            
            Client();
            ~Client();

            // distribute function that takes an event and pushes it into all non null queues

    };

    extern Client* main_client;

}
