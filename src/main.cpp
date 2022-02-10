#include "state_control/controller.hpp"
#include "state_control/guithread.hpp"

int main(int argc, char *argv[])
{
    StateControl::main_ctrl->t_gui.loop(); // opengl + imgui has to run on the main thread
    // destroy the controller so everything cleans up nicely
    StateControl::main_ctrl->~Controller();
    return 0;
}
