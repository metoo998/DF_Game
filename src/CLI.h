#pragma once

#include <string>

struct CLIOptions {
  bool host = false;
  bool join = false;
  bool useAi = false;
  std::string ip = "";
  int port = 7777;
};

class CLI {
public:
  CLIOptions parse(int argc, char** argv) const;
  void printUsage(const char* exeName) const;
};
