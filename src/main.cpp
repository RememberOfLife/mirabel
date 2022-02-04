#include "state_control/controller.hpp"
#include "state_control/enginethread.hpp"
#include "state_control/guithread.hpp"

int main(int argc, char *argv[])
{
    StateControl::main_ctrl->t_engine.start();
    StateControl::main_ctrl->t_gui.loop(); // opengl + imgui has to run on the main thread
    StateControl::main_ctrl->t_engine.stop();
    StateControl::main_ctrl->t_engine.join();
    return 0;
}
