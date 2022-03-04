#include "state_control/guithread.hpp"
#include "state_control/timeout_crash.hpp"

namespace StateControl {

    class Controller {

        public:

            GuiThread t_gui;
            TimeoutCrashThread t_timeout;
            // networkthread
            
            Controller();
            ~Controller();

            // distribute function that takes an event and pushes it into all non null queues

    };

    extern Controller* main_ctrl;

}
