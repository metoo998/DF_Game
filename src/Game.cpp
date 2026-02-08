#include "Game.h"

#include <chrono>
#include <iostream>
#include <thread>

Game::Game(const GameConfig& config)
    : config_(config), network_(config.network) {
  assets_.load();

  if (const auto* strike = cardCatalog_.findById(11)) {
    playerHand_.push_back({strike->id, strike->title, strike->description, strike->effectType});
  }
  if (const auto* shield = cardCatalog_.findById(8)) {
    opponentHand_.push_back({shield->id, shield->title, shield->description, shield->effectType});
  }

  if (config_.useAi) {
    aiClient_.emplace(config_.aiModelPath);
    aiClient_->loadModel();
  }

  setupPlayers();
}

void Game::run() {
  network_.start();

  bool running = true;
  while (running) {
    tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (std::cin.peek() == 'q') {
      running = false;
    }
  }

  network_.shutdown();
}

void Game::tick() {
  runPreparationPhase();
  runPlayPhase();
  runResolutionPhase();
  handleNetworkTick();

  if (aiClient_) {
    aiClient_->evaluateTurn();
  }
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
}

void Game::runResolutionPhase() {
  auto report = rules_.runResolution(0);
  std::cout << "[Phase] " << report.summary << "\n";

  if (!playerHand_.empty()) {
    rules_.applyTypeIIEffect("Time Interference", 0);
    rules_.queueProjectile(playerHand_.front().name, 0);
    rules_.applyTypeIIIEffect("Construction", 0);
  }
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
