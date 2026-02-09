#include "Game.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

Game::Game(const GameConfig& config)
    : config_(config), network_(config.network) {
  assets_.load();

  initializeDeck();
  drawCards(4);

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
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

  rules_.setSystemNeighbors(0, {1});
  rules_.setSystemNeighbors(1, {0, 2});
  rules_.setSystemNeighbors(2, {1});
  rules_.setPlayerSystem(0, 0);
  rules_.setPlayerSystem(1, 2);
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

void Game::initializeDeck() {
  deck_.clear();
  for (const auto& card : cardCatalog_.all()) {
    deck_.push_back({card.id, card.title, card.description, card.effectType});
  }
  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::shuffle(deck_.begin(), deck_.end(), rng);
}

void Game::drawCards(int count) {
  for (int i = 0; i < count; ++i) {
    refillDeckIfNeeded();
    if (deck_.empty()) {
      break;
    }
    playerHand_.push_back(deck_.back());
    deck_.pop_back();
  }
}

void Game::refillDeckIfNeeded() {
  if (!deck_.empty() || discardPile_.empty()) {
    return;
  }
  deck_ = std::move(discardPile_);
  discardPile_.clear();
  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::shuffle(deck_.begin(), deck_.end(), rng);
}

void Game::showHand() const {
  std::cout << "[Hand] " << playerHand_.size() << " cards\n";
  for (size_t i = 0; i < playerHand_.size(); ++i) {
    std::cout << "  [" << i << "] " << playerHand_[i].name << "\n";
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
  if (index >= playerHand_.size()) {
    std::cout << "Invalid hand index.\n";
    return false;
  }
  const auto cardEntry = playerHand_[index];
  pendingActions_[0] = PlayerAction{ActionType::Play, cardEntry.id, targetSystemId,
                                    targetHasCivilization, static_cast<int>(index)};
  pendingReady_[0] = true;
  if (!network_.isHost()) {
    sendActionToHost(pendingActions_[0]);
  }
  return true;
}

bool Game::discardFromHand(size_t index) {
  if (index >= playerHand_.size()) {
    std::cout << "Invalid hand index.\n";
    return false;
  }
  pendingActions_[0] = PlayerAction{ActionType::Discard, playerHand_[index].id, -1, true,
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
    if (action.type == ActionType::Play) {
      const auto* cardDef = cardCatalog_.findById(action.cardId);
      if (cardDef) {
        rules_.resolveCardPlay(*cardDef, static_cast<int>(playerId), action.targetSystemId,
                               action.targetHasCivilization);
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

    if (playerId == 0) {
      if (action.handIndex >= 0 &&
          action.handIndex < static_cast<int>(playerHand_.size())) {
        discardPile_.push_back(playerHand_[static_cast<size_t>(action.handIndex)]);
        playerHand_.erase(playerHand_.begin() + action.handIndex);
        drawCards(1);
      }
    }
  }
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
  return nextId;
}
