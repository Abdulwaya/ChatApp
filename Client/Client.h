#pragma once
#include <SFML/Network.hpp>
#include <string>

class ChatEntity {
protected:
	std::string username;


public:
	virtual void sendMessage(const std::string& msg) = 0;
	virtual ~ChatEntity() = default;
};

class NetworkClient : public ChatEntity {
private:
	sf::TcpSocket socket;


public:
	bool connect(const std::string& name);
	void sendMessage(const std::string& msg) override;
	bool receiveMessage(std::string& msg);
};