#include "Window.h"
#include <type_traits>
#include <filesystem>
#include <fstream>

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

    // Ask for username in console replaced by GUI — initial login false
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
}

void ChatWindow::sendLogin()
{
    if (username.empty()) return;
    sf::Packet p;
    p << std::string("USER") << username << currentRoom;
    if (socket.send(p) != sf::Socket::Status::Done)
        std::cerr << "[CLIENT] Failed to send USER packet\n";
    else
        loggedIn = true;
}

void ChatWindow::sendJoin(const std::string& room)
{
    sf::Packet p;
    p << std::string("JOIN") << room;
    if (socket.send(p) != sf::Socket::Status::Done)
        std::cerr << "[CLIENT] Failed to send JOIN packet\n";
    else
        currentRoom = room;
}

void ChatWindow::sendFileNotification(const std::string& filename)
{
    sf::Packet p;
    p << std::string("FILE") << filename << (std::size_t)0; // filesize 0 for now; rudimentary
    if (socket.send(p) != sf::Socket::Status::Done)
        std::cerr << "[CLIENT] Failed to send FILE packet\n";
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
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void ChatWindow::sendMessage()
{
    if (inputBuffer.empty())
        return;

    // support simple /join and /sendfile commands typed into input
    if (inputBuffer.rfind("/join ", 0) == 0)
    {
        std::string room = inputBuffer.substr(6);
        sendJoin(room);
        inputBuffer.clear();
        return;
    }
    if (inputBuffer.rfind("/sendfile ", 0) == 0)
    {
        std::string path = inputBuffer.substr(10);
        if (std::filesystem::exists(path))
        {
            auto filename = std::filesystem::path(path).filename().string();
            sendFileNotification(filename);

            // Optionally: actually send file bytes (omitted for brevity)
        }
        else
        {
            std::lock_guard<std::mutex> lock(chatMutex);
            chatBuffer += "[SYSTEM] File not found: " + path + "\n";
        }
        inputBuffer.clear();
        return;
    }

    sf::Packet packet;
    packet << std::string("MSG") << inputBuffer;

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

        // Mouse wheel for scrolling
        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
        {
            scrollOffset += wheel->delta * 20.f;
            if (scrollOffset < 0) scrollOffset = 0;
            continue;
        }

        // Text entered
        if (const auto* text = event.getIf<sf::Event::TextEntered>())
        {
            char32_t unicode = text->unicode;

            if (!loggedIn)
            {
                if (unicode == U'\r')
                {
                    // finish login
                    sendLogin();
                }
                else if (unicode == U'\b' && !username.empty())
                    username.pop_back();
                else if (unicode < 128)
                    username += static_cast<char>(unicode);
                continue;
            }

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

    // Apply scroll by changing position
    chatText->setPosition({ 10.f, 10.f - scrollOffset });

    if (!loggedIn)
    {
        // Draw simple login prompt
        sf::Text prompt(font, "Enter username:", 20);
        prompt.setPosition({ 10.f, 200.f });
        window.draw(prompt);

        sf::Text uname(font, username.empty() ? "_" : username, 20);
        uname.setPosition({ 10.f, 240.f });
        window.draw(uname);
    }
    else
    {
        inputText->setString("> " + inputBuffer);
        usernameText->setString("User: " + username + " Room: " + currentRoom);

        window.draw(*chatText);
        window.draw(*usernameText);
        window.draw(*inputText);
    }

    window.display();
}
