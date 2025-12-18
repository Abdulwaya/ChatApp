#include "Window.h"
#include <type_traits>

ChatWindow::ChatWindow()
    : window(sf::VideoMode(sf::Vector2u{ 800, 600 }), "SFML 3.0.2 Chat Client"),
    running(true)
{
    // Load font (must exist beside .exe or provide path)
    if (!font.openFromFile("ARIAL.ttf"))
    {
        std::cerr << "[CLIENT] Failed to load font\n";
    }

    // Create sf::Text objects AFTER font is loaded
    // SFML 3: constructor parameter order is (const sf::Font&, sf::String, unsigned int)
    chatText = std::make_unique<sf::Text>(font, "", 18);
    inputText = std::make_unique<sf::Text>(font, "", 18);
    usernameText = std::make_unique<sf::Text>(font, "User:", 16);

    chatText->setPosition({ 10.f, 10.f });
    usernameText->setPosition({ 10.f, 520.f });
    inputText->setPosition({ 10.f, 550.f });

    // Ask for username in console (can be replaced with GUI later)
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
}

ChatWindow::~ChatWindow()
{
    running = false;
    if (receiveThread.joinable())
        receiveThread.join();
    socket.disconnect();
}

void ChatWindow::run()
{
    connectToServer();

    // Start receive thread
    receiveThread = std::thread(&ChatWindow::receiveLoop, this);

    // Main loop
    while (window.isOpen())
    {
        processEvents();
        render();
    }

    running = false;
}

void ChatWindow::connectToServer()
{
    auto ip = sf::IpAddress::resolve("127.0.0.1");
    if (!ip.has_value())
    {
        std::cerr << "[CLIENT] Failed to resolve server address\n";
        return;
    }

    if (socket.connect(*ip, 53000) != sf::Socket::Status::Done)
    {
        std::cerr << "[CLIENT] Failed to connect to server\n";
        return;
    }

    // Send username
    sf::Packet packet;
    packet << username;
    if (socket.send(packet) != sf::Socket::Status::Done)
    {
        std::cerr << "[CLIENT] Failed to send username\n";
    }
}

void ChatWindow::receiveLoop()
{
    while (running)
    {
        sf::Packet packet;
        if (socket.receive(packet) == sf::Socket::Status::Done)
        {
            std::string msg;
            packet >> msg;

            std::lock_guard<std::mutex> lock(chatMutex);
            chatBuffer += msg + "\n";
        }
    }
}

void ChatWindow::sendMessage()
{
    if (inputBuffer.empty())
        return;

    sf::Packet packet;
    packet << (username + ": " + inputBuffer);

    if (socket.send(packet) != sf::Socket::Status::Done)
        std::cerr << "[CLIENT] Send failed\n";

    inputBuffer.clear();
}

void ChatWindow::processEvents()
{
    while (auto optEvent = window.pollEvent())   // pollEvent() returns optional
    {
        auto& event = *optEvent;                 // unwrap the optional

        // Window closed
        if (event.is<sf::Event::Closed>())
        {
            window.close();
            continue;
        }

        // Text entered
        if (const auto* text = event.getIf<sf::Event::TextEntered>())
        {
            char32_t unicode = text->unicode;

            if (unicode == U'\r')
                sendMessage();
            else if (unicode == U'\b' && !inputBuffer.empty())
                inputBuffer.pop_back();
            else if (unicode < 128)
                inputBuffer += static_cast<char>(unicode);
            continue;
        }

        // Key pressed
        if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Escape)
                window.close();
            continue;
        }

        // ignore other events
    }
}



void ChatWindow::render()
{
    window.clear(sf::Color::Black);

    {
        std::lock_guard<std::mutex> lock(chatMutex);
        chatText->setString(chatBuffer);
    }

    inputText->setString("> " + inputBuffer);
    usernameText->setString("User: " + username);

    window.draw(*chatText);
    window.draw(*usernameText);
    window.draw(*inputText);

    window.display();
}
