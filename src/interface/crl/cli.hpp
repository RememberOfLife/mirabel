#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include "interface/interface.hpp"

class CommandReadLine : public Interface {
  private:

    static const size_t input_size = 2048;
    std::thread input_thread;
    std::mutex input_mut;
    std::condition_variable input_cv;
    char* input_buf;
    bool input_quit;

  public:

    CommandReadLine();

    ~CommandReadLine();

    bool mainloop();

  private:

    void input_thread_func();
};
