#include "Server.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <ctime>

ChatServer::ChatServer() : running(true) {}

void ChatServer::start(unsigned short port) {
    if (listener.listen(port) != sf::Socket::Status::Done)
    {
        std::cerr << "[SERVER] Failed to bind to port " << port << std::endl;
        return;
    }

    // Make listener non-blocking so we can shutdown cleanly
    listener.setBlocking(false);

    std::cout << "[SERVER] Listening on port " << port << std::endl;

    while (running.load()) {
        auto clientSocket = std::make_shared<sf::TcpSocket>();
        if (listener.accept(*clientSocket) == sf::Socket::Status::Done) {
            std::cout << "[SERVER] Client connected (socket)" << std::endl;

            // Handshake: expect a USER packet with username and optional room
            clientSocket->setBlocking(true);
            sf::Packet packet;
            if (clientSocket->receive(packet) == sf::Socket::Status::Done)
            {
                std::string header;
                packet >> header;
                if (header == "USER")
                {
                    std::string username, room;
                    packet >> username >> room; // room may be empty

                    ClientInfo info;
                    info.socket = clientSocket;
                    info.username = username;
                    info.room = room.empty() ? "lobby" : room;

                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        clients.push_back(info);
                    }

                    // Announce to the room with timestamp
                    std::time_t now = std::time(nullptr);
                    char buf[64];
                    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                    std::string joinMsg = std::string("[") + buf + "] " + info.username + " joined the room " + info.room;

                    broadcastToRoom(info.room, joinMsg, clientSocket);

                    // make client socket non-blocking for usual loop
                    clientSocket->setBlocking(false);

                    // Start client handler thread
                    clientThreads.emplace_back(&ChatServer::handleClient, this, info);
                }
                else
                {
                    std::cerr << "[SERVER] Expected USER handshake, got: " << header << std::endl;
                    clientSocket->disconnect();
                }
            }
            else
            {
                std::cerr << "[SERVER] Handshake receive failed", std::endl;
                clientSocket->disconnect();
            }
        }
        else {
            // No incoming connection right now — avoid busy loop
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void ChatServer::handleClient(ClientInfo client)
{
    auto clientSocket = client.socket;
    if (clientSocket)
        clientSocket->setBlocking(false);

    while (running.load())
    {
        sf::Packet packet;

        auto status = clientSocket->receive(packet);
        if (status == sf::Socket::Status::Done)
        {
            std::string header;
            packet >> header;
            if (header == "MSG")
            {
                std::string text;
                packet >> text;
                // prefix with timestamp and username
                std::time_t now = std::time(nullptr);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                std::string out = std::string("[") + buf + "] " + client.username + ": " + text;
                broadcastToRoom(client.room, out);
            }
            else if (header == "JOIN")
            {
                std::string newRoom;
                packet >> newRoom;
                std::string oldRoom = client.room;
                client.room = newRoom.empty() ? "lobby" : newRoom;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    // update stored client entry username/room
                    for (auto &c : clients)
                    {
                        if (c.socket == clientSocket)
                        {
                            c.room = client.room;
                            break;
                        }
                    }
                }
                std::time_t now = std::time(nullptr);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                broadcastToRoom(oldRoom, std::string("[") + buf + "] " + client.username + " left the room");
                broadcastToRoom(client.room, std::string("[") + buf + "] " + client.username + " joined the room");
            }
            else if (header == "FILE")
            {
                // FILE: filename filesize [raw bytes]
                std::string filename;
                std::size_t filesize;
                packet >> filename >> filesize;

                // read raw bytes from packet remainder
                std::vector<std::byte> data;
                data.reserve(filesize);
                // For simplicity, ask client to send file as additional chunk after header; here we forward packet to room

                // Compose a FILE_FWD packet: FILE_FWD filename sender filesize raw
                sf::Packet out;
                out << std::string("FILE_FWD") << filename << client.username << filesize;
                // Attach the raw data from packet if any (not robust but rudimentary)
                // Try to extract remaining bytes (not directly supported by Packet API); so send notification instead
                std::time_t now = std::time(nullptr);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                std::string note = std::string("[") + buf + "] " + client.username + " shared a file: " + filename;
                broadcastToRoom(client.room, note);
            }
            else
            {
                std::cerr << "[SERVER] Unknown header: " << header << std::endl;
            }
        }
        else if (status == sf::Socket::Status::Disconnected)
        {
            std::cout << "[SERVER] Client " << client.username << " disconnected\n";
            removeClient(clientSocket);
            // announce
            std::time_t now = std::time(nullptr);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            broadcastToRoom(client.room, std::string("[") + buf + "] " + client.username + " left the room");
            break;
        }
        else if (status == sf::Socket::Status::NotReady)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        else
        {
            std::cout << "[SERVER] Receive error for client " << client.username << std::endl;
            removeClient(clientSocket);
            break;
        }
    }
}


void ChatServer::broadcastToRoom(const std::string& room, const std::string& msg, const std::shared_ptr<sf::TcpSocket>& except)
{
    std::vector<std::shared_ptr<sf::TcpSocket>> toSend;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto &c : clients)
        {
            if (c.room == room && c.socket && c.socket != except)
                toSend.push_back(c.socket);
        }
    }

    for (auto &s : toSend)
    {
        if (!s) continue;
        sf::Packet p;
        p << msg;
        if (s->send(p) != sf::Socket::Status::Done)
        {
            std::cerr << "[SERVER] Failed to send to a client in room " << room << std::endl;
            removeClient(s);
        }
    }
}

void ChatServer::broadcast(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto &c : clients)
    {
        if (!c.socket) continue;
        sf::Packet p;
        p << msg;
        if (c.socket->send(p) != sf::Socket::Status::Done)
        {
            std::cerr << "[SERVER] Failed to send message to a client\n";
        }
    }
}

void ChatServer::removeClient(const std::shared_ptr<sf::TcpSocket>& clientSocket)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    if (clientSocket)
        clientSocket->disconnect();

    clients.erase(
        std::remove_if(clients.begin(), clients.end(), [&](const ClientInfo &ci) { return ci.socket == clientSocket; }),
        clients.end()
    );
}



void ChatServer::sendMessage(const std::string& msg) {
    broadcast(msg);
}


void ChatServer::stop() {
    running.store(false);
    listener.close();
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto &c : clients) if (c.socket) c.socket->disconnect();
    clients.clear();
}


ChatServer::~ChatServer() {
    stop();
    for (auto& t : clientThreads) {
        if (t.joinable()) t.join();
    }
}