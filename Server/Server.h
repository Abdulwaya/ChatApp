#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <string>

class ChatEntity {
protected:
	std::string username;


public:
	virtual void sendMessage(const std::string& msg) = 0;
	virtual ~ChatEntity() = default;
};

class ChatServer : public ChatEntity {
private:
	sf::TcpListener listener;
	std::vector<std::shared_ptr<sf::TcpSocket>> clients;
	std::vector<std::thread> clientThreads;
	std::mutex clientsMutex;
	void removeClient(const std::shared_ptr<sf::TcpSocket>& client);
	bool running;


	void handleClient(std::shared_ptr<sf::TcpSocket> client);


public:
	ChatServer();
	void start(unsigned short port);
	void broadcast(const std::string& msg);
	void sendMessage(const std::string& msg) override;
	~ChatServer();
};