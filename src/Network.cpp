#include "Network.h"

#include <iostream>

NetworkSession::NetworkSession(const NetworkConfig& config) : config_(config) {}

void NetworkSession::start() {
  if (config_.role == NetworkRole::Host) {
    std::cout << "[Network] Hosting on port " << config_.port << "\n";
  } else {
    std::cout << "[Network] Connecting to " << config_.ip << ":" << config_.port << "\n";
  }
}

void NetworkSession::tick() {
  // TODO: Implement socket IO.
}

void NetworkSession::shutdown() {
  std::cout << "[Network] Session closed.\n";
}
