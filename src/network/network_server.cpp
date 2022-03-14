#include "network/network_server.hpp"

namespace Network {

    NetworkServer::NetworkServer()
    {
        
    }

    NetworkServer::~NetworkServer()
    {
        
    }

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
