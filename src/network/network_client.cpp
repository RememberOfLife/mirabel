#include "SDL_net.h"

#include "network/network_client.hpp"

namespace Network {

    NetworkClient::NetworkClient()
    {
        socketset = SDLNet_AllocSocketSet(1);
    }

    void NetworkClient::send_loop()
    {
        while(1) {}
    }

    void NetworkClient::recv_loop()
    {
        while(1) {}
    }
    
    void NetworkClient::start()
    {
        send_runner = std::thread(&NetworkClient::send_loop, this);
        recv_runner = std::thread(&NetworkClient::recv_loop, this);
    }
    
    void NetworkClient::join()
    {
        send_runner.join();
        recv_runner.join();
    }
    
}
