#include "network/network_server.hpp"

namespace Network {

    void NetworkServer::loop()
    {
        while(1) {}
    }
    
    void NetworkServer::start()
    {
        runner = std::thread(&NetworkServer::loop, this);
    }
    
    void NetworkServer::join()
    {
        runner.join();
    }
    
}
