#include "GameRules.h"

#include <iostream>

PhaseReport GameRules::runPreparation(int playerId) {
  updateProjectiles();
  resolveTypeI();
  updatePlayerParams(playerId);
  updateTypeIII();
  return {Phase::Preparation,
          "Preparation: projectiles advanced, Type I resolved, params/Type III updated."};
}

PhaseReport GameRules::runPlay(int playerId) {
  (void)playerId;
  return {Phase::Play, "Play: simultaneous discard/play decisions (hidden)."};
}

PhaseReport GameRules::runResolution(int playerId) {
  (void)playerId;
  return {Phase::Resolution, "Resolution: apply Type II, spawn projectiles, spawn Type III."};
}

ObservationReport GameRules::generateObservations(int playerId) {
  (void)playerId;
  ObservationReport report;
  report.systems.push_back({1, "Star present", 0});
  report.systems.push_back({2, "Star present", 1});
  report.warnings.push_back({1, "Incoming projectile within distance 1."});
  return report;
}

void GameRules::queueProjectile(const std::string& cardName, int ownerId) {
  projectiles_.push_back({cardName, 1, ownerId});
  std::cout << "[Rules] Queued projectile from card: " << cardName << "\n";
}

void GameRules::applyTypeIIEffect(const std::string& cardName, int ownerId) {
  std::cout << "[Rules] Applying Type II effect from card: " << cardName
            << " by player " << ownerId << "\n";
}

void GameRules::applyTypeIIIEffect(const std::string& cardName, int ownerId) {
  std::cout << "[Rules] Applying Type III effect from card: " << cardName
            << " by player " << ownerId << "\n";
}

void GameRules::addPlayer(int playerId) {
  if (playerId >= static_cast<int>(players_.size())) {
    players_.resize(playerId + 1);
  }
}

void GameRules::updateProjectiles() {
  for (auto& projectile : projectiles_) {
    if (projectile.remainingTime > 0) {
      projectile.remainingTime -= 1;
    }
  }
}

void GameRules::resolveTypeI() {
  std::cout << "[Rules] Resolving Type I effects (projectiles).\n";
}

void GameRules::updateTypeIII() {
  std::cout << "[Rules] Updating Type III effects (buildings/planet states).\n";
}

void GameRules::updatePlayerParams(int playerId) {
  addPlayer(playerId);
  auto& params = players_[playerId];
  params.energy += 1;
  params.draw = 1;
  if (params.cooldown > 0) {
    params.cooldown -= 1;
  }
}
