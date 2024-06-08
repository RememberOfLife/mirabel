#pragma once

class Interface {
  public:

    Interface(){};

    virtual ~Interface(){};

    virtual bool mainloop() = 0;
};
