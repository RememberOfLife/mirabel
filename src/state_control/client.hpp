#include "state_control/guithread.hpp"
#include "state_control/timeout_crash.hpp"

namespace StateControl {

    class Client {

        public:

            TimeoutCrashThread t_timeout;
            GuiThread t_gui;
            // networkthread
            
            Client();
            ~Client();

            // distribute function that takes an event and pushes it into all non null queues

    };

    extern Client* main_client;

}
