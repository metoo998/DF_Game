#include "Game.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <thread>

Game::Game(const GameConfig& config)
    : config_(config), network_(config.network) {
  assets_.load();

  playerStates_.assign(static_cast<size_t>(config_.network.maxPlayers), PlayerState{});
  aiControlled_.assign(static_cast<size_t>(config_.network.maxPlayers), false);

  if (config_.useAi) {
    aiClient_.emplace(config_.aiModelPath);
    aiClient_->loadModel();
  }

  setupPlayers();
}

void Game::run() {
  network_.start();

  while (running_) {
    runPreparationPhase();
    runPlayPhase();
    runResolutionPhase();

    for (const auto& message : network_.receiveMessages()) {
      std::cout << "[Chat] " << message.payload << "\n";
    }

    network_.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  network_.shutdown();
}

void Game::runPreparationPhase() {
  auto report = rules_.runPreparation(0);
  std::cout << "[Phase] " << report.summary << "\n";
}

void Game::runPlayPhase() {
  auto observations = rules_.generateObservations(0);
  for (const auto& system : observations.systems) {
    std::cout << "[Observe] System " << system.systemId << ": " << system.starStatus
              << " (age " << system.ageInRounds << ")\n";
  }
  for (const auto& warning : observations.warnings) {
    std::cout << "[Warning] System " << warning.systemId << ": " << warning.message << "\n";
  }

  auto report = rules_.runPlay(0);
  std::cout << "[Phase] " << report.summary << "\n";

  pendingActions_.assign(static_cast<size_t>(config_.network.maxPlayers), PlayerAction{});
  pendingReady_.assign(static_cast<size_t>(config_.network.maxPlayers), false);
  pendingAcks_.assign(static_cast<size_t>(config_.network.maxPlayers), false);

  showHand();
  std::cout << "Actions: play <index> <targetSystem> [hasCiv 0|1], discard <index>, pass, hand, say <msg>, end, quit\n";
  while (running_) {
    std::cout << "action> ";
    std::string line;
    if (!std::getline(std::cin, line)) {
      running_ = false;
      break;
    }
    if (line == "end") {
      handlePlayCommand("pass");
      break;
    }
    if (handlePlayCommand(line)) {
      break;
    }
  }

  if (!running_) {
    return;
  }

  if (network_.isHost()) {
    int waitTicks = 0;
    while (running_) {
      collectRemoteActions();
      bool allReady = true;
      for (size_t i = 0; i < pendingReady_.size(); ++i) {
        if (!pendingReady_[i]) {
          allReady = false;
          break;
        }
      }
      if (allReady) {
        resolvePendingActions();
        updateWinCondition();
        for (size_t i = 1; i < pendingAcks_.size(); ++i) {
          pendingAcks_[i] = false;
        }
        int ackTicks = 0;
        while (running_) {
          collectRemoteActions();
          bool allAcked = true;
          for (size_t i = 1; i < pendingAcks_.size(); ++i) {
            if (!pendingAcks_[i]) {
              allAcked = false;
              break;
            }
          }
          if (allAcked || ackTicks >= 40) {
            break;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          ++ackTicks;
        }
        break;
      }
      if (waitTicks >= 80) {
        markMissingReadyAsPass();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      ++waitTicks;
    }
  } else {
    bool sawReveal = false;
    int waitTicks = 0;
    while (running_) {
      for (const auto& message : network_.receiveMessages()) {
        if (message.payload.rfind("REVEAL ", 0) == 0) {
          std::cout << "[Reveal] " << message.payload.substr(7) << "\n";
          sawReveal = true;
        }
        if (message.payload.rfind("REVEAL_END", 0) == 0) {
          network_.sendRawMessage("ACK");
          sawReveal = true;
        }
      }
      if (sawReveal || waitTicks >= 40) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      ++waitTicks;
    }
  }
}

void Game::runResolutionPhase() {
  auto report = rules_.runResolution(0);
  std::cout << "[Phase] " << report.summary << "\n";
}

void Game::setupPlayers() {
  // TODO: Initialize multiplayer players, deck lists, and ready states.
  std::cout << "[Multiplayer Placeholder] Players configured.\n";
  std::cout << "[Assets Placeholder] UI slots: \n";
  for (const auto& slot : assets_.placeholders()) {
    std::cout << "  - " << slot.label << " -> " << slot.pathHint << "\n";
  }
  std::cout << "[Card Catalog] Loaded cards: " << cardCatalog_.all().size() << "\n";
  std::cout << "[Role Catalog] Loaded roles: " << roleCatalog_.all().size() << "\n";

  SurvivalConfig survivalConfig;
  survivalConfig.enableRevival = true;
  survivalConfig.revivalBuildingCardIds = {8, 9, 10};
  survivalConfig.allowMigrationRevival = true;
  survivalConfig.allowTimeInterferenceRevival = true;
  rules_.setSurvivalConfig(survivalConfig);

  for (int i = 0; i < config_.network.maxPlayers; ++i) {
    initializePlayerDeck(i);
    drawCards(i, 4);
  }

  rules_.setSystemNeighbors(0, {1});
  rules_.setSystemNeighbors(1, {0, 2});
  rules_.setSystemNeighbors(2, {1});
  rules_.setPlayerSystem(0, 0);
  if (config_.network.maxPlayers > 1) {
    rules_.setPlayerSystem(1, 2);
  }
}

void Game::handleNetworkTick() {
  network_.tick();
}

void Game::playCard(const Card& card) {
  // TODO: Apply card effects, animations, and network sync.
  card.applyEffect();
}

void Game::resolveTurn() {
  // TODO: Resolve combat and update game state.
}

void Game::initializePlayerDeck(int playerId) {
  if (playerId < 0 || playerId >= static_cast<int>(playerStates_.size())) {
    return;
  }
  auto& state = playerStates_[playerId];
  state.deck.clear();
  state.discard.clear();
  std::vector<CardDefinition> pool;
  const CardDefinition* harmony = nullptr;
  for (const auto& card : cardCatalog_.all()) {
    if (card.id == 17) {
      harmony = &card;
      continue;
    }
    pool.push_back(card);
  }
  if (harmony) {
    state.deck.push_back({harmony->id, harmony->title, harmony->description, harmony->effectType});
  }

  std::vector<int> weights;
  weights.reserve(pool.size());
  for (const auto& card : pool) {
    int weight = std::max(1, 10 - card.cost);
    weights.push_back(weight);
  }

  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()) +
                   static_cast<unsigned>(playerId));
  std::discrete_distribution<int> picker(weights.begin(), weights.end());
  const int targetDeckSize = 60;
  while (static_cast<int>(state.deck.size()) < targetDeckSize) {
    int index = picker(rng);
    const auto& card = pool[static_cast<size_t>(index)];
    state.deck.push_back({card.id, card.title, card.description, card.effectType});
  }
  std::shuffle(state.deck.begin(), state.deck.end(), rng);
}

void Game::drawCards(int playerId, int count) {
  if (playerId < 0 || playerId >= static_cast<int>(playerStates_.size())) {
    return;
  }
  auto& state = playerStates_[playerId];
  for (int i = 0; i < count; ++i) {
    refillDeckIfNeeded(playerId);
    if (state.deck.empty()) {
      break;
    }
    state.hand.push_back(state.deck.back());
    state.deck.pop_back();
  }
}

void Game::refillDeckIfNeeded(int playerId) {
  if (playerId < 0 || playerId >= static_cast<int>(playerStates_.size())) {
    return;
  }
  auto& state = playerStates_[playerId];
  if (!state.deck.empty() || state.discard.empty()) {
    return;
  }
  state.deck = std::move(state.discard);
  state.discard.clear();
  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()) +
                   static_cast<unsigned>(playerId));
  std::shuffle(state.deck.begin(), state.deck.end(), rng);
}

void Game::showHand() const {
  if (playerStates_.empty()) {
    return;
  }
  const auto& hand = playerStates_[0].hand;
  std::cout << "[Hand] " << hand.size() << " cards\n";
  for (size_t i = 0; i < hand.size(); ++i) {
    std::cout << "  [" << i << "] " << hand[i].name << "\n";
  }
}

bool Game::handlePlayCommand(const std::string& line) {
  if (line == "hand") {
    showHand();
    return false;
  }
  if (line == "quit") {
    network_.sendMessage("has left the game.");
    running_ = false;
    return true;
  }
  if (line.rfind("say ", 0) == 0) {
    std::string message = line.substr(4);
    if (!message.empty()) {
      network_.sendMessage(message);
    }
    return false;
  }

  std::istringstream stream(line);
  std::string command;
  stream >> command;
  if (command == "play") {
    size_t index = 0;
    int targetSystemId = 0;
    int hasCiv = 1;
    if (!(stream >> index >> targetSystemId)) {
      std::cout << "Usage: play <index> <targetSystem> [hasCiv 0|1]\n";
      return false;
    }
    bool result = false;
    if (stream >> hasCiv) {
      result = playFromHand(index, targetSystemId, hasCiv != 0);
    } else {
      result = playFromHand(index, targetSystemId, true);
    }
    return result;
  }
  if (command == "discard") {
    size_t index = 0;
    if (!(stream >> index)) {
      std::cout << "Usage: discard <index>\n";
      return false;
    }
    return discardFromHand(index);
  }
  if (command == "pass") {
    pendingActions_[0] = PlayerAction{ActionType::Pass, -1, -1, true, -1};
    pendingReady_[0] = true;
    if (!network_.isHost()) {
      sendActionToHost(pendingActions_[0]);
    }
    return true;
  }

  std::cout << "Unknown command.\n";
  return false;
}

bool Game::playFromHand(size_t index, int targetSystemId, bool targetHasCivilization) {
  if (playerStates_.empty() || index >= playerStates_[0].hand.size()) {
    std::cout << "Invalid hand index.\n";
    return false;
  }
  const auto cardEntry = playerStates_[0].hand[index];
  pendingActions_[0] = PlayerAction{ActionType::Play, cardEntry.id, targetSystemId,
                                    targetHasCivilization, static_cast<int>(index)};
  pendingReady_[0] = true;
  if (!network_.isHost()) {
    sendActionToHost(pendingActions_[0]);
  }
  return true;
}

bool Game::discardFromHand(size_t index) {
  if (playerStates_.empty() || index >= playerStates_[0].hand.size()) {
    std::cout << "Invalid hand index.\n";
    return false;
  }
  pendingActions_[0] = PlayerAction{ActionType::Discard, playerStates_[0].hand[index].id, -1, true,
                                    static_cast<int>(index)};
  pendingReady_[0] = true;
  if (!network_.isHost()) {
    sendActionToHost(pendingActions_[0]);
  }
  return true;
}

void Game::collectRemoteActions() {
  for (const auto& message : network_.receiveMessages()) {
    if (message.payload.rfind("ACT ", 0) == 0) {
      std::istringstream stream(message.payload);
      std::string act;
      std::string type;
      PlayerAction action;
      int hasCiv = 1;
      stream >> act >> type >> action.cardId >> action.targetSystemId >> hasCiv;
      action.targetHasCivilization = hasCiv != 0;
      if (type == "play") {
        action.type = ActionType::Play;
      } else if (type == "discard") {
        action.type = ActionType::Discard;
      } else if (type == "pass") {
        action.type = ActionType::Pass;
      }
      int playerId = ensureRemotePlayerId(message.sender);
      if (playerId >= 0 && playerId < static_cast<int>(pendingActions_.size())) {
        pendingActions_[playerId] = action;
      }
      continue;
    }
    if (message.payload.rfind("READY", 0) == 0) {
      int playerId = ensureRemotePlayerId(message.sender);
      if (playerId >= 0 && playerId < static_cast<int>(pendingReady_.size())) {
        pendingReady_[playerId] = true;
        aiControlled_[playerId] = false;
      }
      continue;
    }
    if (message.payload.rfind("ACK", 0) == 0) {
      int playerId = ensureRemotePlayerId(message.sender);
      if (playerId >= 0 && playerId < static_cast<int>(pendingAcks_.size())) {
        pendingAcks_[playerId] = true;
      }
      continue;
    }
    if (message.payload.rfind("REVEAL ", 0) == 0) {
      std::cout << "[Reveal] " << message.payload.substr(7) << "\n";
      continue;
    }
    std::cout << "[Chat] " << message.payload << "\n";
  }
}

void Game::resolvePendingActions() {
  for (size_t playerId = 0; playerId < pendingActions_.size(); ++playerId) {
    const auto& action = pendingActions_[playerId];
    if (action.type == ActionType::None) {
      continue;
    }

    std::string actionSummary;
    if (!validateAndApplyAction(static_cast<int>(playerId), action)) {
      actionSummary = "player " + std::to_string(playerId) + " action invalid -> pass";
    } else if (action.type == ActionType::Play) {
      const auto* cardDef = cardCatalog_.findById(action.cardId);
      if (cardDef) {
        actionSummary = "player " + std::to_string(playerId) + " played " + cardDef->title;
      }
    } else if (action.type == ActionType::Discard) {
      actionSummary = "player " + std::to_string(playerId) + " discarded a card";
    } else if (action.type == ActionType::Pass) {
      actionSummary = "player " + std::to_string(playerId) + " passed";
    }

    if (!actionSummary.empty()) {
      network_.sendRawMessage("REVEAL " + actionSummary);
      std::cout << "[Reveal] " << actionSummary << "\n";
    }
  }
  network_.sendRawMessage("REVEAL_END");
  rules_.resolveSurvival();
}

void Game::sendActionToHost(const PlayerAction& action) {
  std::string type = "pass";
  if (action.type == ActionType::Play) {
    type = "play";
  } else if (action.type == ActionType::Discard) {
    type = "discard";
  }
  std::string payload = "ACT " + type + " " + std::to_string(action.cardId) + " " +
                        std::to_string(action.targetSystemId) + " " +
                        std::to_string(action.targetHasCivilization ? 1 : 0);
  network_.sendRawMessage(payload);
  network_.sendRawMessage("READY");
}

int Game::ensureRemotePlayerId(const std::string& name) {
  auto it = remotePlayerIds_.find(name);
  if (it != remotePlayerIds_.end()) {
    return it->second;
  }
  int nextId = static_cast<int>(remotePlayerIds_.size()) + 1;
  if (nextId >= static_cast<int>(pendingActions_.size())) {
    return -1;
  }
  remotePlayerIds_[name] = nextId;
  if (nextId >= static_cast<int>(playerStates_.size())) {
    return nextId;
  }
  if (playerStates_[nextId].deck.empty() && playerStates_[nextId].hand.empty()) {
    initializePlayerDeck(nextId);
    drawCards(nextId, 4);
  }
  return nextId;
}

bool Game::validateAndApplyAction(int playerId, const PlayerAction& action) {
  if (playerId < 0 || playerId >= static_cast<int>(playerStates_.size())) {
    return false;
  }
  auto& state = playerStates_[playerId];
  if (action.type == ActionType::Pass) {
    return true;
  }
  if (action.type != ActionType::Play && action.type != ActionType::Discard) {
    return false;
  }

  auto findById = [&](int cardId) -> int {
    for (size_t i = 0; i < state.hand.size(); ++i) {
      if (state.hand[i].id == cardId) {
        return static_cast<int>(i);
      }
    }
    return -1;
  };

  int handIndex = findById(action.cardId);
  if (handIndex == -1) {
    return false;
  }

  if (action.type == ActionType::Play) {
    const auto* cardDef = cardCatalog_.findById(action.cardId);
    if (!cardDef) {
      return false;
    }
    if (!rules_.resolveCardPlay(*cardDef, playerId, action.targetSystemId,
                                action.targetHasCivilization)) {
      return false;
    }
  }

  state.discard.push_back(state.hand[static_cast<size_t>(handIndex)]);
  state.hand.erase(state.hand.begin() + handIndex);
  drawCards(playerId, 1);
  return true;
}

void Game::markMissingReadyAsPass() {
  for (size_t i = 0; i < pendingReady_.size(); ++i) {
    if (!pendingReady_[i]) {
      pendingActions_[i] = PlayerAction{ActionType::Pass, -1, -1, true, -1};
      pendingReady_[i] = true;
      aiControlled_[i] = true;
    }
  }
}

void Game::updateWinCondition() {
  auto players = rules_.players();
  int aliveCount = 0;
  int lastAlive = -1;
  for (size_t i = 0; i < players.size(); ++i) {
    if (players[i].alive) {
      ++aliveCount;
      lastAlive = static_cast<int>(i);
    }
  }
  if (aliveCount == 1) {
    std::string summary = "Winner: player " + std::to_string(lastAlive);
    network_.sendRawMessage("REVEAL " + summary);
    std::cout << "[Reveal] " << summary << "\n";
    running_ = false;
  } else if (aliveCount == 0) {
    std::string summary = "No winner (all eliminated).";
    network_.sendRawMessage("REVEAL " + summary);
    std::cout << "[Reveal] " << summary << "\n";
    running_ = false;
  }
}
