#pragma once

#include "interface/interface.hpp"

class GraphicalImmediateMode : public Interface {
  public:

    GraphicalImmediateMode();

    ~GraphicalImmediateMode();

    bool mainloop();
};
