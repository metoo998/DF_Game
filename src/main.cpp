#include "CLI.h"
#include "Game.h"

#include <iostream>

int main(int argc, char** argv) {
  CLI cli;
  CLIOptions options = cli.parse(argc, argv);

  if (!options.host && !options.join) {
    cli.printUsage(argv[0]);
    return 1;
  }

  if (options.join && options.ip.empty()) {
    std::cerr << "--join requires --ip <address>\n";
    return 1;
  }

  GameConfig config{
      .network = {
          .role = options.host ? NetworkRole::Host : NetworkRole::Client,
          .ip = options.ip,
          .port = options.port,
          .maxPlayers = options.players,
          .playerName = options.name,
      },
      .useAi = options.useAi,
  };

  Game game(config);
  std::cout << "Type messages to chat, or 'quit' to exit.\n";
  game.run();
  return 0;
}
