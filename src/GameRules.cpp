#include "GameRules.h"
#include "Cards.h"

#include <chrono>
#include <iostream>
#include <random>

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
  addSystem(1);
  addSystem(2);
  report.systems.push_back({1, system(1).starExists ? "Star present" : "Star absent", 0});
  report.systems.push_back({2, system(2).isDimensionalized ? "2D collapse" : "Star present", 1});
  report.warnings.push_back({1, "Incoming projectile within distance 1."});
  return report;
}

void GameRules::addSystem(int systemId) {
  if (systemId >= static_cast<int>(systems_.size())) {
    systems_.resize(systemId + 1);
  }
}

const SystemState& GameRules::system(int systemId) const {
  return systems_.at(systemId);
}

bool GameRules::resolveCardPlay(const CardDefinition& card, int playerId, int targetSystemId,
                                bool targetHasCivilization) {
  addPlayer(playerId);
  if (!spendEnergy(playerId, card.cost)) {
    std::cout << "[Rules] Not enough energy to play " << card.title << ".\n";
    return false;
  }

  switch (card.category) {
    case CardCategory::Broadcast:
    case CardCategory::Strike: {
      int level = card.level;
      queueProjectile(card.title, playerId, targetSystemId, level, card.id);
      if (card.id == 14) {
        applyTechLockdown(targetSystemId, playerId);
      }
      if (card.id == 18) {
        applyInterstellarExpedition(targetSystemId, playerId);
      }
      break;
    }
    case CardCategory::Energy:
    case CardCategory::Defense:
    case CardCategory::Building:
    case CardCategory::Special:
      addBuilding(playerId, card.id);
      if (card.id == 8) {
        players_[playerId].defenseLevel = std::max(players_[playerId].defenseLevel, 2);
      } else if (card.id == 9) {
        players_[playerId].defenseLevel = std::max(players_[playerId].defenseLevel, 3);
      }
      break;
    case CardCategory::Skill:
      if (card.id == 19) {
        applyTimeInterference(targetSystemId, playerId, targetHasCivilization);
      }
      break;
  }

  std::cout << "[Rules] Played card: " << card.title << " by player " << playerId << ".\n";
  return true;
}

void GameRules::queueProjectile(const std::string& cardName, int ownerId, int targetSystemId,
                                int level, int cardId) {
  projectiles_.push_back({cardName, 1, ownerId, targetSystemId, level, cardId});
  std::cout << "[Rules] Queued projectile from card: " << cardName << " -> system "
            << targetSystemId << "\n";
}

void GameRules::applyTypeIIEffect(const std::string& cardName, int ownerId) {
  std::cout << "[Rules] Applying Type II effect from card: " << cardName
            << " by player " << ownerId << "\n";
}

void GameRules::applyTypeIIIEffect(const std::string& cardName, int ownerId) {
  std::cout << "[Rules] Applying Type III effect from card: " << cardName
            << " by player " << ownerId << "\n";
}

void GameRules::applyTimeInterference(int systemId, int ownerId, bool targetHasCivilization) {
  addSystem(systemId);
  if (!targetHasCivilization) {
    std::cout << "[Rules] Time Interference had no effect on empty/colony system " << systemId
              << ".\n";
    return;
  }

  auto& target = systems_[systemId];
  target.starExists = false;
  target.destroyed = true;
  target.isDimensionalized = false;
  target.occupied = false;
  target.colonized = false;
  std::cout << "[Rules] Time Interference erased civilization in system " << systemId
            << " by player " << ownerId << ".\n";
}

void GameRules::applyTechLockdown(int systemId, int ownerId) {
  addSystem(systemId);
  std::cout << "[Rules] Tech Lockdown discards building cards in system " << systemId
            << " by player " << ownerId << ".\n";
}

void GameRules::applyInterstellarExpedition(int systemId, int ownerId) {
  addSystem(systemId);
  std::cout << "[Rules] Interstellar Expedition launched toward system " << systemId
            << " by player " << ownerId << ".\n";
}

void GameRules::addPlayer(int playerId) {
  if (playerId >= static_cast<int>(players_.size())) {
    players_.resize(playerId + 1);
  }
}

void GameRules::setBroadcastResponseChoice(int playerId, const std::string& choice) {
  addPlayer(playerId);
  if (choice == "cooperate" || choice == "stealth") {
    players_[playerId].pendingBroadcastChoice = choice;
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
  for (auto& projectile : projectiles_) {
    if (projectile.remainingTime != 0) {
      continue;
    }
    if (projectile.cardId == 1 || projectile.cardId == 2 || projectile.cardId == 3 ||
        projectile.cardId == 101 || projectile.cardId == 102 || projectile.cardId == 103) {
      resolveBroadcastProjectile(projectile);
    } else {
      resolveStrikeProjectile(projectile);
    }
    projectile.remainingTime = -1;
  }
}

void GameRules::resolveStrikeProjectile(const Projectile& projectile) {
  addSystem(projectile.targetSystemId);
  auto& target = systems_[projectile.targetSystemId];

  if (projectile.cardId == 14) {
    applyTechLockdown(projectile.targetSystemId, projectile.ownerId);
    return;
  }
  if (projectile.cardId == 18) {
    applyInterstellarExpedition(projectile.targetSystemId, projectile.ownerId);
    return;
  }
  if (projectile.cardId == 15) {
    target.destroyed = true;
    target.starExists = false;
    target.isDimensionalized = true;
    return;
  }
  if (projectile.cardId == 12 || projectile.cardId == 13) {
    target.starExists = false;
  }
  if (projectile.level > 0) {
    target.destroyed = true;
  }
}

void GameRules::resolveBroadcastProjectile(const Projectile& projectile) {
  addSystem(projectile.targetSystemId);
  std::vector<int> responders;
  for (size_t playerId = 0; playerId < players_.size(); ++playerId) {
    if (static_cast<int>(playerId) == projectile.ownerId) {
      continue;
    }
    if (playerHasListeningBase(static_cast<int>(playerId))) {
      continue;
    }
    responders.push_back(static_cast<int>(playerId));
  }

  std::cout << "[Rules] Broadcast resolved for system " << projectile.targetSystemId
            << " from player " << projectile.ownerId << ". Responders=" << responders.size()
            << "\n";

  if (responders.empty()) {
    players_[projectile.ownerId].energy += 1;
    return;
  }

  std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::uniform_int_distribution<size_t> pick(0, responders.size() - 1);
  int responder = responders[pick(rng)];

  std::string broadcasterChoice = broadcastVariant(projectile);
  std::string responderChoice = players_[responder].pendingBroadcastChoice.empty()
                                    ? "cooperate"
                                    : players_[responder].pendingBroadcastChoice;

  if (broadcasterChoice == "cooperate" && responderChoice == "cooperate") {
    players_[projectile.ownerId].energy += 3;
    players_[responder].energy += 3;
  } else if (broadcasterChoice == "stealth" && responderChoice == "stealth") {
    // no energy change
  } else {
    if (broadcasterChoice == "stealth") {
      players_[projectile.ownerId].energy += 5;
    } else {
      players_[responder].energy += 5;
    }
  }

  players_[responder].pendingBroadcastChoice.clear();
}

bool GameRules::playerHasListeningBase(int playerId) const {
  if (playerId < 0 || playerId >= static_cast<int>(playerBuildings_.size())) {
    return false;
  }
  for (int cardId : playerBuildings_[playerId]) {
    if (cardId == 16) {
      return true;
    }
  }
  return false;
}

std::string GameRules::broadcastVariant(const Projectile& projectile) const {
  if (projectile.cardId == 101 || projectile.cardId == 102 || projectile.cardId == 103) {
    return "stealth";
  }
  return "cooperate";
}
void GameRules::updateTypeIII() {
  for (size_t playerId = 0; playerId < playerBuildings_.size(); ++playerId) {
    int energyGain = 0;
    for (int cardId : playerBuildings_[playerId]) {
      switch (cardId) {
        case 4:
        case 5:
          energyGain += 1;
          break;
        case 6:
          energyGain += 2;
          break;
        case 7:
          energyGain += 3;
          break;
        case 17:
          energyGain += 7;
          break;
        default:
          break;
      }
    }
    players_[playerId].energy += energyGain;
  }
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

bool GameRules::spendEnergy(int playerId, int cost) {
  addPlayer(playerId);
  if (players_[playerId].energy < cost) {
    return false;
  }
  players_[playerId].energy -= cost;
  return true;
}

void GameRules::addBuilding(int playerId, int cardId) {
  addPlayer(playerId);
  if (playerId >= static_cast<int>(playerBuildings_.size())) {
    playerBuildings_.resize(playerId + 1);
  }
  playerBuildings_[playerId].push_back(cardId);
}
