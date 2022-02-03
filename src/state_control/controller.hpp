#include "state_control/enginethread.hpp"
#include "state_control/guithread.hpp"

namespace StateControl {

    class Controller {

        public:

            GuiThread t_gui;
            EngineThread t_engine;
            // networkthread
            
            Controller();

    };

    extern Controller* main_ctrl;

}
