#pragma once
#include <SFML/Network.hpp>
#include <string>

//
// Network client header: declares NetworkClient which encapsulates a
// single TCP socket used by the client application to communicate
// with the chat server.
//

// Base interface for chat-capable entities used by both client & server.
class ChatEntity {
protected:
	std::string username;

public:
	// Send a textual message (interface only).
	virtual void sendMessage(const std::string& msg) = 0;
	virtual ~ChatEntity() = default;
};

// NetworkClient:
// - Manages a single sf::TcpSocket instance.
class NetworkClient : public ChatEntity {
private:
	// Underlying TCP socket used to communicate with server.
	sf::TcpSocket socket;

public:
	// Connect to the server and set the username locally.
	// Returns true on success.
	bool connect(const std::string& name);

	// Send a chat message. The username is prefixed in the payload.
	void sendMessage(const std::string& msg) override;

	// Try to receive a message from the server. Returns true
	// only when a complete packet was read.
	bool receiveMessage(std::string& msg);
};