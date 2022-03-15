#pragma once

#include "network/network_client.hpp"
#include "control/guithread.hpp"
#include "control/event_queue.hpp"
#include "control/timeout_crash.hpp"

namespace Control {

    class Client {

        public:

            TimeoutCrashThread t_timeout;
            GuiThread t_gui;
            Network::NetworkClient* t_network = NULL;
            event_queue* network_send_queue = NULL;
            // offline server likely somewhere here
            
            Client();
            ~Client();

            // distribute function that takes an event and pushes it into all non null queues

    };

    extern Client* main_client;

}
