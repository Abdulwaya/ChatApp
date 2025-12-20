#include "Server.h"
#include <iostream>

// Server implementation
// - Accept connections
// - Spawn a per-client receiver thread
// - Broadcast incoming messages to all clients

// Constructor: initialize running flag.
ChatServer::ChatServer() : running(true) {}

// start:
// - Bind the listener to the provided port.
// - Enter an accept loop that spawns a new thread for each accepted client.
// - The server stores shared_ptr sockets in `clients` and protects the list
//   with `clientsMutex`.
void ChatServer::start(unsigned short port) {
	if (listener.listen(port) != sf::Socket::Status::Done)
	{
		std::cerr << "[SERVER] Failed to bind to port " << port << std::endl;
		return;
	}

	std::cout << "[SERVER] Listening on port " << port << std::endl;

	while (running) {
		auto client = std::make_shared<sf::TcpSocket>();

		// Accept a new connection (blocking), add to list and
		// start a handler thread. 
		if (listener.accept(*client) == sf::Socket::Status::Done) {
			std::cout << "[SERVER] Client connected" << std::endl;

			{
				// Add the new client to the clients list under lock.
				std::lock_guard<std::mutex> lock(clientsMutex);
				clients.push_back(client);
			}

			// Spawn a thread to handle receiving from this client.
			clientThreads.emplace_back(&ChatServer::handleClient, this, client);
		}
	}
}

// handleClient:
// - Runs on a per-client thread.
// - Receives packets from the client and broadcasts the extracted string
//   message to all other clients.
// - On disconnect or receive error, removes the client from the list and exits.
void ChatServer::handleClient(std::shared_ptr<sf::TcpSocket> client)
{
	while (running)
	{
		sf::Packet packet;

		// Blocking receive on the client's socket. If a complete packet is
		// received, extract a string and broadcast it.
		if (client->receive(packet) == sf::Socket::Status::Done)
		{
			std::string msg;
			packet >> msg;

			// Print to server stdout for monitoring
			std::cout << msg << std::endl;

			// Relay to all connected clients
			broadcast(msg);
		}
		else
		{
			// Client disconnected or error occurred: remove it and stop this thread.
			std::cout << "[SERVER] Client disconnected\n";
			removeClient(client);
			break;
		}
	}
}

// broadcast:
// - Sends `msg` to each client in the `clients` list.
void ChatServer::broadcast(const std::string& msg)
{
	std::vector<std::shared_ptr<sf::TcpSocket>> clientsCopy;

	// Lock only to copy the client list
	{
		std::lock_guard<std::mutex> lock(clientsMutex);
		clientsCopy = clients;
	}

	// Send without holding the mutex
	for (auto& client : clientsCopy)
	{
		sf::Packet packet;
		packet << msg;

		if (client->send(packet) != sf::Socket::Status::Done)
		{
			// Log send failure.
			std::cerr << "[SERVER] Failed to send message to a client\n";
		}
	}
}

// removeClient:
// - Erases the specified client socket from the server's client list
//   while holding the mutex.
void ChatServer::removeClient(const std::shared_ptr<sf::TcpSocket>& client)
{
	std::lock_guard<std::mutex> lock(clientsMutex);

	clients.erase(
		std::remove(clients.begin(), clients.end(), client),
		clients.end()
	);
}

void ChatServer::sendMessage(const std::string& msg) {
	broadcast(msg);
}

// Destructor: stop server and join threads.
ChatServer::~ChatServer() {
	running = false;
	for (auto& t : clientThreads) {
		if (t.joinable()) t.join();
	}
}