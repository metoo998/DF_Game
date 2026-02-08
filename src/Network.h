#pragma once

#include <string>

enum class NetworkRole {
  Host,
  Client
};

struct NetworkConfig {
  NetworkRole role;
  std::string ip;
  int port = 7777;
};

class NetworkSession {
public:
  explicit NetworkSession(const NetworkConfig& config);
  void start();
  void tick();
  void shutdown();

private:
  NetworkConfig config_;
};
