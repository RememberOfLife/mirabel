#include "network/network_client.hpp"

namespace Network {

    void NetworkClient::loop()
    {
        while(1) {}
    }
    
    void NetworkClient::start()
    {
        runner = std::thread(&NetworkClient::loop, this);
    }
    
    void NetworkClient::join()
    {
        runner.join();
    }
    
}
