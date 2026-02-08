#include "Network.h"

#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
using SocketType = SOCKET;
constexpr SocketType kInvalidSocket = INVALID_SOCKET;
#else
using SocketType = int;
constexpr SocketType kInvalidSocket = -1;
#endif

void closeSocketPlatform(SocketType socket) {
#ifdef _WIN32
  closesocket(socket);
#else
  close(socket);
#endif
}
}

NetworkSession::NetworkSession(const NetworkConfig& config) : config_(config) {}

NetworkSession::~NetworkSession() {
  shutdown();
}

void NetworkSession::initSockets() {
#ifdef _WIN32
  WSADATA data;
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }
#endif
}

void NetworkSession::cleanupSockets() {
#ifdef _WIN32
  WSACleanup();
#endif
}

void NetworkSession::start() {
  initSockets();
  running_ = true;

  if (config_.role == NetworkRole::Host) {
    listenSocket_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (listenSocket_ == kInvalidSocket) {
      throw std::runtime_error("Failed to create listen socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(config_.port));

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
      throw std::runtime_error("Bind failed");
    }
    if (listen(listenSocket_, config_.maxPlayers) < 0) {
      throw std::runtime_error("Listen failed");
    }

    std::cout << "[Network] Hosting on port " << config_.port << " (waiting for "
              << config_.maxPlayers << " players)\n";
    acceptConnections();
  } else {
    int clientSocket = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (clientSocket == kInvalidSocket) {
      throw std::runtime_error("Failed to create client socket");
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(config_.port));
#ifdef _WIN32
    InetPton(AF_INET, config_.ip.c_str(), &server.sin_addr);
#else
    inet_pton(AF_INET, config_.ip.c_str(), &server.sin_addr);
#endif

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
      throw std::runtime_error("Connect failed");
    }

    hostConnection_.socket = clientSocket;
    hostConnection_.name = "Host";
    sendLine(hostConnection_.socket, "JOIN " + config_.playerName);
    std::cout << "[Network] Connected to " << config_.ip << ":" << config_.port << "\n";
  }
}

void NetworkSession::acceptConnections() {
  connections_.clear();
  int toAccept = config_.maxPlayers - 1;
  while (toAccept > 0) {
    sockaddr_in clientAddr{};
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
#else
    socklen_t addrLen = sizeof(clientAddr);
#endif
    int clientSocket = static_cast<int>(accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen));
    if (clientSocket == kInvalidSocket) {
      throw std::runtime_error("Accept failed");
    }
    Connection connection{};
    connection.socket = clientSocket;
    connection.name = "Player" + std::to_string(static_cast<int>(connections_.size()) + 1);
    connections_.push_back(connection);
    std::cout << "[Network] Player connected. Waiting for " << (toAccept - 1) << " more...\n";
    --toAccept;
  }
  std::cout << "[Network] All players connected.\n";
}

void NetworkSession::tick() {
  // Placeholder for future network tick logic.
}

void NetworkSession::shutdown() {
  if (!running_) {
    return;
  }
  running_ = false;

  if (listenSocket_ != kInvalidSocket) {
    closeSocketPlatform(listenSocket_);
    listenSocket_ = kInvalidSocket;
  }

  if (hostConnection_.socket != kInvalidSocket) {
    closeSocketPlatform(hostConnection_.socket);
    hostConnection_.socket = kInvalidSocket;
  }

  for (auto& connection : connections_) {
    if (connection.socket != kInvalidSocket) {
      closeSocketPlatform(connection.socket);
      connection.socket = kInvalidSocket;
    }
  }

  cleanupSockets();
}

bool NetworkSession::isHost() const {
  return config_.role == NetworkRole::Host;
}

void NetworkSession::sendLine(int socket, const std::string& message) {
  std::string line = message + "\n";
  send(socket, line.c_str(), static_cast<int>(line.size()), 0);
}

void NetworkSession::broadcast(const std::string& message, int excludeSocket) {
  for (auto& connection : connections_) {
    if (connection.socket != kInvalidSocket && connection.socket != excludeSocket) {
      sendLine(connection.socket, message);
    }
  }
}

void NetworkSession::sendMessage(const std::string& message) {
  if (config_.role == NetworkRole::Host) {
    broadcast("MSG " + config_.playerName + ": " + message);
  } else if (hostConnection_.socket != kInvalidSocket) {
    sendLine(hostConnection_.socket, "MSG " + config_.playerName + ": " + message);
  }
}

std::vector<NetworkMessage> NetworkSession::drainSocket(Connection& connection) {
  std::vector<NetworkMessage> messages;
  char buffer[512];
  int received = recv(connection.socket, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    return messages;
  }
  buffer[received] = '\0';
  connection.buffer += buffer;

  size_t pos = 0;
  while ((pos = connection.buffer.find('\n')) != std::string::npos) {
    std::string line = connection.buffer.substr(0, pos);
    connection.buffer.erase(0, pos + 1);

    if (line.rfind("JOIN ", 0) == 0) {
      connection.name = line.substr(5);
      messages.push_back({"System", connection.name + " joined."});
      continue;
    }

    if (line.rfind("MSG ", 0) == 0) {
      std::string payload = line.substr(4);
      messages.push_back({connection.name, payload});
      continue;
    }

    if (!line.empty()) {
      messages.push_back({connection.name, line});
    }
  }

  return messages;
}

std::vector<NetworkMessage> NetworkSession::receiveMessages() {
  std::vector<NetworkMessage> messages;

  if (config_.role == NetworkRole::Host) {
    for (auto& connection : connections_) {
      if (connection.socket == kInvalidSocket) {
        continue;
      }
      fd_set readSet;
      FD_ZERO(&readSet);
      FD_SET(connection.socket, &readSet);
      timeval timeout{};
      int ready = select(connection.socket + 1, &readSet, nullptr, nullptr, &timeout);
      if (ready > 0 && FD_ISSET(connection.socket, &readSet)) {
        auto drained = drainSocket(connection);
        for (auto& msg : drained) {
          messages.push_back(msg);
          broadcast("MSG " + msg.sender + ": " + msg.payload, connection.socket);
        }
      }
    }
  } else if (hostConnection_.socket != kInvalidSocket) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(hostConnection_.socket, &readSet);
    timeval timeout{};
    int ready = select(hostConnection_.socket + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready > 0 && FD_ISSET(hostConnection_.socket, &readSet)) {
      auto drained = drainSocket(hostConnection_);
      for (auto& msg : drained) {
        messages.push_back(msg);
      }
    }
  }

  return messages;
}
