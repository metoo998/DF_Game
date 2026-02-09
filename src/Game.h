#pragma once

#include "AiClient.h"
#include "Assets.h"
#include "Card.h"
#include "Cards.h"
#include "GameRules.h"
#include "Network.h"
#include "Roles.h"
#include <optional>
#include <unordered_map>
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
  enum class ActionType {
    None,
    Play,
    Discard,
    Pass
  };

  struct PlayerAction {
    ActionType type = ActionType::None;
    int cardId = -1;
    int targetSystemId = -1;
    bool targetHasCivilization = true;
    int handIndex = -1;
  };

  struct PlayerState {
    std::vector<Card> hand{};
    std::vector<Card> deck{};
    std::vector<Card> discard{};
    bool connected = true;
  };

  void tick();

  void runPreparationPhase();
  void runPlayPhase();
  void runResolutionPhase();
  void initializePlayerDeck(int playerId);
  void drawCards(int playerId, int count);
  void refillDeckIfNeeded(int playerId);
  void showHand() const;
  bool handlePlayCommand(const std::string& line);
  bool playFromHand(size_t index, int targetSystemId, bool targetHasCivilization);
  bool discardFromHand(size_t index);
  void collectRemoteActions();
  void resolvePendingActions();
  void sendActionToHost(const PlayerAction& action);
  int ensureRemotePlayerId(const std::string& name);
  bool validateAndApplyAction(int playerId, const PlayerAction& action);
  void markMissingReadyAsPass();

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
  std::vector<PlayerState> playerStates_{};
  bool running_ = true;
  std::vector<PlayerAction> pendingActions_{};
  std::vector<bool> pendingReady_{};
  std::vector<bool> pendingAcks_{};
  std::unordered_map<std::string, int> remotePlayerIds_{};
  std::optional<AiClient> aiClient_;
};
