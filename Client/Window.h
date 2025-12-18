#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <type_traits>
#include <vector>

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

    // GUI/login state
    bool loggedIn = false;
    std::string currentRoom = "lobby";

    // Scrolling
    float scrollOffset = 0.f;

    // File transfer helper (rudimentary)
    std::string pendingSendFilePath;

    // Private methods
    void receiveLoop();
    void sendMessage();
    void sendLogin();
    void sendJoin(const std::string& room);
    void sendFileNotification(const std::string& filename);

public:
    ChatWindow();
    ~ChatWindow();

    void run();               // Main loop
    void processEvents();     // Handle SFML events
    void render();            // Draw everything
    void connectToServer();   // Connect to TCP server
};
