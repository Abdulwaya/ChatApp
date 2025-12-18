#include "Client.h"


bool NetworkClient::connect(const std::string& name) {
	username = name;
	return socket.connect(sf::IpAddress::LocalHost, 53000) == sf::Socket::Status::Done;
}


void NetworkClient::sendMessage(const std::string& msg) {
	sf::Packet packet;
	packet << username + ": " + msg;
	socket.send(packet);
}


bool NetworkClient::receiveMessage(std::string& msg) {
	sf::Packet packet;
	if (socket.receive(packet) == sf::Socket::Status::Done) {
		packet >> msg;
		return true;
	}
	return false;
}