#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <type_traits>

class ChatWindow
{
private:
    // SFML window and resources
    sf::RenderWindow window;
    sf::Font font;  // Must be a class member for Text lifetime
    std::unique_ptr<sf::Text> chatText;
    std::unique_ptr<sf::Text> inputText;
    std::unique_ptr<sf::Text> usernameText;

    // Networking
    sf::TcpSocket socket;

    // Threading
    std::thread receiveThread;
    std::mutex chatMutex;

    // Chat data
    std::string username;
    std::string inputBuffer;
    std::string chatBuffer;
    bool running;

    // Private methods
    void receiveLoop();
    void sendMessage();

public:
    ChatWindow();
    ~ChatWindow();

    void run();               // Main loop
    void processEvents();     // Handle SFML events
    void render();            // Draw everything
    void connectToServer();   // Connect to TCP server
};
