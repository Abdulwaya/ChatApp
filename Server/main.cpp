#include <iostream>
#include "Server.h"

int main()
{
    constexpr unsigned short PORT = 53000;

    std::cout << "=====================================\n";
    std::cout << "   SFML 3.0.2 Multithreaded Chat Server\n";
    std::cout << "=====================================\n";
    std::cout << "[SERVER] Starting on port " << PORT << "...\n";

    ChatServer server;
    server.start(PORT);

    std::cout << "[SERVER] Server stopped.\n";
    return 0;
}
