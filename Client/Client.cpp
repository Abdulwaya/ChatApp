#include "Client.h"


// connect:
// - Stores the provided username locally, then attempts to connect to
//   the server at localhost on port 53000.
// - Returns true on successful connection.
bool NetworkClient::connect(const std::string& name) {
	username = name;
	return socket.connect(sf::IpAddress::LocalHost, 53000) == sf::Socket::Status::Done;
}

// sendMessage:
// - Packages `username: msg` into an sf::Packet and sends it to the server.
// - This operation is synchronous; error handling is minimal here.
void NetworkClient::sendMessage(const std::string& msg) {
	sf::Packet packet;
	packet << username + ": " + msg;
	socket.send(packet);
}

// receiveMessage:
// - Attempts to receive a packet from the server. If successful, extracts
//   the contained string into `msg` and returns true.
bool NetworkClient::receiveMessage(std::string& msg) {
	sf::Packet packet;
	if (socket.receive(packet) == sf::Socket::Status::Done) {
		packet >> msg;
		return true;
	}
	return false;
}