#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <string>

/*
Server header: declares ChatServer which accepts TCP clients and
broadcasts messages to all connected clients.
Base interface for chat-capable entities.
Concrete classes implement sendMessage to deliver text to recipients.
*/
class ChatEntity {
protected:
	std::string username;

public:
	// Send a textual message (interface only).
	virtual void sendMessage(const std::string& msg) = 0;
	virtual ~ChatEntity() = default;
};

// ChatServer accepts TCP connections from clients and relays messages.
// - Maintains a list of connected client sockets.
// - Spawns a thread per connected client to receive data.
// - Provides broadcast to push messages to all clients.
class ChatServer : public ChatEntity {
private:
	// Listener used to accept incoming TCP connections.
	sf::TcpListener listener;

	// Connected client sockets.
	std::vector<std::shared_ptr<sf::TcpSocket>> clients;

	// Threads handling per-client receive loops.
	std::vector<std::thread> clientThreads;

	// Protects `clients`.
	std::mutex clientsMutex;

	// Remove a client socket from the internal list (called on disconnect).
	void removeClient(const std::shared_ptr<sf::TcpSocket>& client);

	// Flag used by loops to keep the server running.
	bool running;

	// Per-client receive loop.
	void handleClient(std::shared_ptr<sf::TcpSocket> client);

public:
	// Construct a server instance (does not bind or listen).
	ChatServer();

	// Start listening on the given port and accept clients.
	// This function blocks until the server is stopped or an error occurs.
	void start(unsigned short port);

	// Broadcast a message to all connected clients. Each client receives
	// the same string encoded in an sf::Packet.
	void broadcast(const std::string& msg);

	void sendMessage(const std::string& msg) override;

	// Clean up: stop threads and close sockets.
	~ChatServer();
};