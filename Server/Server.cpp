#include "Server.h"
#include <iostream>


ChatServer::ChatServer() : running(true) {}


void ChatServer::start(unsigned short port) {
	if (listener.listen(port) != sf::Socket::Status::Done)
	{
		std::cerr << "[SERVER] Failed to bind to port " << port << std::endl;
		return;
	}

	std::cout << "[SERVER] Listening on port " << port << std::endl;


	while (running) {
		auto client = std::make_shared<sf::TcpSocket>();
		if (listener.accept(*client) == sf::Socket::Status::Done) {
			std::cout << "[SERVER] Client connected" << std::endl;


			{
				std::lock_guard<std::mutex> lock(clientsMutex);
				clients.push_back(client);
			}


			clientThreads.emplace_back(&ChatServer::handleClient, this, client);
		}
	}
}


void ChatServer::handleClient(std::shared_ptr<sf::TcpSocket> client)
{
	while (running)
	{
		sf::Packet packet;

		if (client->receive(packet) == sf::Socket::Status::Done)
		{
			std::string msg;
			packet >> msg;
			std::cout << msg << std::endl;
			broadcast(msg);
		}
		else
		{
			std::cout << "[SERVER] Client disconnected\n";
			removeClient(client);
			break;
		}
	}
}



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
			std::cerr << "[SERVER] Failed to send message to a client\n";
		}
	}
}

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


ChatServer::~ChatServer() {
	running = false;
	for (auto& t : clientThreads) {
		if (t.joinable()) t.join();
	}
}