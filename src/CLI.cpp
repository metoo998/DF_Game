#include "CLI.h"

#include <iostream>

CLIOptions CLI::parse(int argc, char** argv) const {
  CLIOptions options{};
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--host") {
      options.host = true;
    } else if (arg == "--join") {
      options.join = true;
    } else if (arg == "--ip" && i + 1 < argc) {
      options.ip = argv[++i];
    } else if (arg == "--port" && i + 1 < argc) {
      options.port = std::stoi(argv[++i]);
    } else if (arg == "--ai") {
      options.useAi = true;
    } else if (arg == "--help") {
      printUsage(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      std::exit(1);
    }
  }
  return options;
}

void CLI::printUsage(const char* exeName) const {
  std::cout << "Usage: " << exeName << " [--host | --join --ip <address>] [--port <port>] [--ai]\n"
            << "  --host          Create a room and act as server.\n"
            << "  --join          Join a room by IP. Requires --ip.\n"
            << "  --ip            Host IP address to join.\n"
            << "  --port          Port to use (default 7777).\n"
            << "  --ai            Enable AI player on host.\n";
}
