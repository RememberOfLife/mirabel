#pragma once

#include "interface/interface.hpp"

class CommandReadLine : public Interface {
  public:

    CommandReadLine();

    ~CommandReadLine();

    bool mainloop();
};
