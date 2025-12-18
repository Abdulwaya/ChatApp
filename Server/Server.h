#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>

class ChatEntity {
protected:
    std::string username;


public:
    virtual void sendMessage(const std::string& msg) = 0;
    virtual ~ChatEntity() = default;
};

struct ClientInfo {
    std::shared_ptr<sf::TcpSocket> socket;
    std::string username;
    std::string room;
};

class ChatServer : public ChatEntity {
private:
    sf::TcpListener listener;
    std::vector<ClientInfo> clients;
    std::vector<std::thread> clientThreads;
    std::mutex clientsMutex;
    void removeClient(const std::shared_ptr<sf::TcpSocket>& clientSocket);
    std::atomic_bool running;


    void handleClient(ClientInfo client);


public:
    ChatServer();
    void start(unsigned short port);
    void stop();
    void broadcastToRoom(const std::string& room, const std::string& msg, const std::shared_ptr<sf::TcpSocket>& except = nullptr);
    void broadcast(const std::string& msg);
    void sendMessage(const std::string& msg) override;
    ~ChatServer();
};