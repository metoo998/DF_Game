#pragma once

#include <string>
#include <vector>

enum class NetworkRole {
  Host,
  Client
};

struct NetworkConfig {
  NetworkRole role;
  std::string ip;
  int port = 7777;
  int maxPlayers = 2;
  std::string playerName = "Player";
};

struct NetworkMessage {
  std::string sender;
  std::string payload;
};

class NetworkSession {
public:
  explicit NetworkSession(const NetworkConfig& config);
  ~NetworkSession();

  void start();
  void tick();
  void shutdown();

  void sendMessage(const std::string& message);
  void sendRawMessage(const std::string& message);
  std::vector<NetworkMessage> receiveMessages();
  bool isHost() const;

private:
  struct Connection {
    int socket = -1;
    std::string buffer;
    std::string name = "";
  };

  void initSockets();
  void cleanupSockets();
  void acceptConnections();
  void closeSocket(int socket);
  void broadcast(const std::string& message, int excludeSocket = -1);
  void sendLine(int socket, const std::string& message);
  std::vector<NetworkMessage> drainSocket(Connection& connection);

  NetworkConfig config_;
  bool running_ = false;
  int listenSocket_ = -1;
  Connection hostConnection_{};
  std::vector<Connection> connections_{};
};
