#include "state_control/guithread.hpp"

namespace StateControl {

    class Controller {

        public:

            GuiThread t_gui;
            // networkthread
            
            Controller();
            ~Controller();

            // distribute function that takes an event and pushes it into all non null queues

    };

    extern Controller* main_ctrl;

}
