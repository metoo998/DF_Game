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

  showHand();
  std::cout << "Actions: play <index> <targetSystem> [hasCiv 0|1], discard <index>, hand, say <msg>, end, quit\n";
  while (running_) {
    std::cout << "action> ";
    std::string line;
    if (!std::getline(std::cin, line)) {
      running_ = false;
      break;
    }
    if (line == "end") {
      break;
    }
    if (handlePlayCommand(line)) {
      break;
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
    if (stream >> hasCiv) {
      return playFromHand(index, targetSystemId, hasCiv != 0);
    }
    return playFromHand(index, targetSystemId, true);
  }
  if (command == "discard") {
    size_t index = 0;
    if (!(stream >> index)) {
      std::cout << "Usage: discard <index>\n";
      return false;
    }
    return discardFromHand(index);
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
  const auto* cardDef = cardCatalog_.findById(cardEntry.id);
  if (!cardDef) {
    std::cout << "Card not found.\n";
    return false;
  }
  if (!rules_.resolveCardPlay(*cardDef, 0, targetSystemId, targetHasCivilization)) {
    return false;
  }
  discardPile_.push_back(cardEntry);
  playerHand_.erase(playerHand_.begin() + static_cast<long>(index));
  drawCards(1);
  return true;
}

bool Game::discardFromHand(size_t index) {
  if (index >= playerHand_.size()) {
    std::cout << "Invalid hand index.\n";
    return false;
  }
  discardPile_.push_back(playerHand_[index]);
  playerHand_.erase(playerHand_.begin() + static_cast<long>(index));
  drawCards(1);
  return true;
}
