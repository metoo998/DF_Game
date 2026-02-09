#pragma once

#include "AiClient.h"
#include "Assets.h"
#include "Card.h"
#include "Cards.h"
#include "GameRules.h"
#include "Network.h"
#include "Roles.h"
#include <optional>
#include <vector>

struct GameConfig {
  NetworkConfig network;
  bool useAi = false;
  std::string aiModelPath = "models/ai/default.model";
};

class Game {
public:
  explicit Game(const GameConfig& config);
  void run();

private:
  void tick();

  void runPreparationPhase();
  void runPlayPhase();
  void runResolutionPhase();
  void initializeDeck();
  void drawCards(int count);
  void refillDeckIfNeeded();
  void showHand() const;
  bool handlePlayCommand(const std::string& line);
  bool playFromHand(size_t index, int targetSystemId, bool targetHasCivilization);
  bool discardFromHand(size_t index);

  // Placeholder hooks for multiplayer flow.
  void setupPlayers();
  void handleNetworkTick();

  // Placeholder hooks for card interactions.
  void playCard(const Card& card);
  void resolveTurn();

  GameConfig config_;
  NetworkSession network_;
  Assets assets_;
  GameRules rules_;
  CardCatalog cardCatalog_;
  RoleCatalog roleCatalog_;
  std::vector<Card> playerHand_{};
  std::vector<Card> opponentHand_{};
  std::vector<Card> deck_{};
  std::vector<Card> discardPile_{};
  bool running_ = true;
  std::optional<AiClient> aiClient_;
};
