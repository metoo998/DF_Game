#include "GameRules.h"
#include "Cards.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>

PhaseReport GameRules::runPreparation(int playerId) {
  if (pendingTimeInterferenceReset_) {
    resetPlayersForTimeInterference(timeInterferenceOwnerId_);
    pendingTimeInterferenceReset_ = false;
    timeInterferenceOwnerId_ = -1;
  }
  updateProjectiles();
  resolveTypeI();
  updatePlayerParams(playerId);
  updateTypeIII();
  return {Phase::Preparation,
          "Preparation: projectiles advanced, Type I resolved, params/Type III updated."};
}

PhaseReport GameRules::runPlay(int playerId) {
  addPlayer(playerId);
  if (players_[playerId].skipPlayRounds > 0) {
    players_[playerId].skipPlayRounds -= 1;
    return {Phase::Play, "Play: skipped due to expedition failure."};
  }
  return {Phase::Play, "Play: simultaneous discard/play decisions (hidden)."};
}

PhaseReport GameRules::runResolution(int playerId) {
  (void)playerId;
  return {Phase::Resolution, "Resolution: apply Type II, spawn projectiles, spawn Type III."};
}

ObservationReport GameRules::generateObservations(int playerId) {
  addPlayer(playerId);
  ObservationReport report;
  addSystem(1);
  addSystem(2);
  std::vector<ObservedSystemState> combined;
  std::vector<int> seenSystems;
  auto appendUnique = [&](const std::vector<ObservedSystemState>& snapshot) {
    for (const auto& systemState : snapshot) {
      if (std::find(seenSystems.begin(), seenSystems.end(), systemState.systemId) !=
          seenSystems.end()) {
        continue;
      }
      seenSystems.push_back(systemState.systemId);
      combined.push_back(systemState);
    }
  };

  appendUnique(buildObservationSnapshot(playerId, ""));
  for (size_t otherId = 0; otherId < players_.size(); ++otherId) {
    if (players_[otherId].sharesObservationWith == playerId) {
      appendUnique(buildObservationSnapshot(static_cast<int>(otherId), "Shared: "));
    }
  }

  report.systems = std::move(combined);
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

void GameRules::setSystemNeighbors(int systemId, const std::vector<int>& neighbors) {
  addSystem(systemId);
  systems_[systemId].neighbors = neighbors;
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
      int expeditionEnergy = 0;
      if (card.id == 18) {
        int extraEnergy = consumePendingExpeditionEnergy(playerId);
        if (extraEnergy > 0 && !spendEnergy(playerId, extraEnergy)) {
          players_[playerId].energy += card.cost;
          std::cout << "[Rules] Not enough energy for Interstellar Expedition extra cost.\n";
          return false;
        }
        expeditionEnergy = card.cost + extraEnergy;
      }
      queueProjectile(card.title, playerId, targetSystemId, level, card.id, expeditionEnergy);
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
                                int level, int cardId, int expeditionEnergy) {
  projectiles_.push_back({cardName, 1, ownerId, targetSystemId, level, cardId, expeditionEnergy});
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
  target.ownerId = -1;
  target.occupierId = -1;
  pendingTimeInterferenceReset_ = true;
  timeInterferenceOwnerId_ = ownerId;
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
  if (playerId >= static_cast<int>(playerSystems_.size())) {
    playerSystems_.resize(playerId + 1, -1);
  }
}

void GameRules::setBroadcastResponseChoice(int playerId, const std::string& choice) {
  addPlayer(playerId);
  if (choice == "cooperate" || choice == "stealth") {
    players_[playerId].pendingBroadcastChoice = choice;
  }
}

void GameRules::setExpeditionEnergyChoice(int playerId, int energy) {
  addPlayer(playerId);
  if (energy == 0 || energy == 5 || energy == 10 || energy == 20) {
    players_[playerId].pendingExpeditionEnergy = energy;
  }
}

void GameRules::setExpeditionDefenseChoice(int playerId, const std::string& choice) {
  addPlayer(playerId);
  if (choice == "fight" || choice == "surrender") {
    players_[playerId].pendingExpeditionDefenseChoice = choice;
  }
}

void GameRules::setPlayerSystem(int playerId, int systemId) {
  addPlayer(playerId);
  playerSystems_[playerId] = systemId;
  addSystem(systemId);
  systems_[systemId].ownerId = playerId;
}

int GameRules::playerSystem(int playerId) const {
  if (playerId < 0 || playerId >= static_cast<int>(playerSystems_.size())) {
    return -1;
  }
  return playerSystems_[playerId];
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
    resolveInterstellarExpedition(projectile);
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
  int broadcastRange = 1;
  if (projectile.cardId == 2 || projectile.cardId == 102) {
    broadcastRange = 2;
  } else if (projectile.cardId == 3 || projectile.cardId == 103) {
    broadcastRange = -1;
  }

  std::vector<int> responders;
  for (size_t playerId = 0; playerId < players_.size(); ++playerId) {
    if (static_cast<int>(playerId) == projectile.ownerId) {
      continue;
    }
    if (playerHasListeningBase(static_cast<int>(playerId))) {
      continue;
    }
    int responderSystem = playerSystem(static_cast<int>(playerId));
    if (responderSystem == -1) {
      continue;
    }
    if (broadcastRange >= 0 &&
        !isSystemWithinDistance(projectile.targetSystemId, responderSystem, broadcastRange)) {
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

void GameRules::resolveInterstellarExpedition(const Projectile& projectile) {
  addSystem(projectile.targetSystemId);
  int defenderId = firstPlayerInSystem(projectile.targetSystemId, projectile.ownerId);

  if (defenderId == -1) {
    applyColonization(projectile.targetSystemId, projectile.ownerId, -1);
    std::cout << "[Rules] Interstellar Expedition colonized empty system "
              << projectile.targetSystemId << ".\n";
    return;
  }

  std::string defenseChoice = consumeExpeditionDefenseChoice(defenderId);
  if (defenseChoice.empty()) {
    defenseChoice = "fight";
  }

  int attackerInvestment = projectile.expeditionEnergy > 0 ? projectile.expeditionEnergy : 4;
  int defenderEnergyAtDecision = players_[defenderId].energy;

  if (defenseChoice == "surrender") {
    int tribute = players_[defenderId].energy / 2;
    players_[defenderId].energy -= tribute;
    players_[projectile.ownerId].energy += tribute;
    players_[defenderId].sharesObservationWith = projectile.ownerId;
    applyOccupation(projectile.targetSystemId, projectile.ownerId, defenderId);
    std::cout << "[Rules] Expedition surrender: defender " << defenderId
              << " paid tribute and is now occupied.\n";
    return;
  }

  int defenseCost = attackerInvestment / 2;
  if (players_[defenderId].energy >= defenseCost) {
    players_[defenderId].energy -= defenseCost;
  }

  if (defenderEnergyAtDecision > attackerInvestment) {
    players_[projectile.ownerId].skipPlayRounds = 1;
    players_[projectile.ownerId].revealedPosition = true;
    std::cout << "[Rules] Expedition failed: attacker " << projectile.ownerId
              << " skipped next play phase and revealed position.\n";
    return;
  }

  players_[defenderId].nearDeath = true;
  std::cout << "[Rules] Expedition battle lost: defender " << defenderId
            << " is in near-death state.\n";
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

int GameRules::consumePendingExpeditionEnergy(int playerId) {
  addPlayer(playerId);
  int energy = players_[playerId].pendingExpeditionEnergy;
  players_[playerId].pendingExpeditionEnergy = 0;
  if (energy == 0 || energy == 5 || energy == 10 || energy == 20) {
    return energy;
  }
  return 0;
}

std::string GameRules::consumeExpeditionDefenseChoice(int playerId) {
  addPlayer(playerId);
  std::string choice = players_[playerId].pendingExpeditionDefenseChoice;
  players_[playerId].pendingExpeditionDefenseChoice.clear();
  if (choice == "fight" || choice == "surrender") {
    return choice;
  }
  return "";
}

int GameRules::firstPlayerInSystem(int systemId, int excludePlayerId) const {
  for (size_t playerId = 0; playerId < playerSystems_.size(); ++playerId) {
    if (static_cast<int>(playerId) == excludePlayerId) {
      continue;
    }
    if (playerSystems_[playerId] == systemId && players_[playerId].alive) {
      return static_cast<int>(playerId);
    }
  }
  return -1;
}

void GameRules::applyOccupation(int systemId, int occupierId, int ownerId) {
  addSystem(systemId);
  auto& target = systems_[systemId];
  target.occupied = true;
  target.colonized = false;
  target.occupierId = occupierId;
  target.ownerId = ownerId;
}

void GameRules::applyColonization(int systemId, int colonizerId, int ownerId) {
  addSystem(systemId);
  auto& target = systems_[systemId];
  target.colonized = true;
  target.occupied = false;
  target.occupierId = colonizerId;
  target.ownerId = ownerId;
}

int GameRules::computeProductionForPlayer(int playerId) const {
  if (playerId < 0 || playerId >= static_cast<int>(playerBuildings_.size())) {
    return 0;
  }
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
  return energyGain;
}

std::vector<ObservedSystemState> GameRules::buildObservationSnapshot(
    int playerId, const std::string& prefix) const {
  (void)playerId;
  std::vector<ObservedSystemState> snapshot;
  if (systems_.size() <= 2) {
    return snapshot;
  }
  const auto& systemOne = systems_[1];
  const auto& systemTwo = systems_[2];
  snapshot.push_back(
      {1, prefix + (systemOne.starExists ? "Star present" : "Star absent"), 0});
  snapshot.push_back(
      {2, prefix + (systemTwo.isDimensionalized ? "2D collapse" : "Star present"), 1});
  return snapshot;
}

void GameRules::resetPlayersForTimeInterference(int ownerId) {
  for (size_t playerId = 0; playerId < players_.size(); ++playerId) {
    if (static_cast<int>(playerId) == ownerId) {
      continue;
    }
    players_[playerId] = PlayerParams{};
    players_[playerId].alive = true;
    players_[playerId].sharesObservationWith = -1;
  }

  for (auto& systemState : systems_) {
    systemState.occupied = false;
    systemState.colonized = false;
    systemState.occupierId = -1;
  }

  for (size_t playerId = 0; playerId < playerBuildings_.size(); ++playerId) {
    if (static_cast<int>(playerId) == ownerId) {
      continue;
    }
    playerBuildings_[playerId].clear();
  }

  projectiles_.clear();
}

bool GameRules::isSystemWithinDistance(int startSystemId, int targetSystemId,
                                       int distance) const {
  if (distance < 0) {
    return true;
  }
  if (startSystemId == targetSystemId) {
    return true;
  }
  if (startSystemId < 0 || targetSystemId < 0 || startSystemId >= static_cast<int>(systems_.size()) ||
      targetSystemId >= static_cast<int>(systems_.size())) {
    return false;
  }

  std::vector<int> frontier{startSystemId};
  std::vector<int> visited(systems_.size(), -1);
  visited[startSystemId] = 0;

  while (!frontier.empty()) {
    std::vector<int> next;
    for (int node : frontier) {
      int depth = visited[node];
      if (depth >= distance) {
        continue;
      }
      for (int neighbor : systems_[node].neighbors) {
        if (neighbor < 0 || neighbor >= static_cast<int>(systems_.size())) {
          continue;
        }
        if (visited[neighbor] != -1) {
          continue;
        }
        visited[neighbor] = depth + 1;
        if (neighbor == targetSystemId) {
          return true;
        }
        next.push_back(neighbor);
      }
    }
    frontier.swap(next);
  }

  return false;
}
void GameRules::updateTypeIII() {
  std::vector<int> production(players_.size(), 0);
  for (size_t playerId = 0; playerId < players_.size(); ++playerId) {
    production[playerId] = computeProductionForPlayer(static_cast<int>(playerId));
  }

  for (const auto& system : systems_) {
    if (system.occupierId == -1 || system.ownerId == -1) {
      continue;
    }
    if (system.ownerId >= static_cast<int>(production.size()) ||
        system.occupierId >= static_cast<int>(production.size())) {
      continue;
    }
    if (system.occupied) {
      int transfer = production[system.ownerId] / 2;
      production[system.ownerId] -= transfer;
      production[system.occupierId] += transfer;
    } else if (system.colonized) {
      int transfer = production[system.ownerId];
      production[system.ownerId] -= transfer;
      production[system.occupierId] += transfer;
    }
  }

  for (size_t playerId = 0; playerId < production.size(); ++playerId) {
    players_[playerId].energy += production[playerId];
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

void GameRules::resolveSurvival() {
  for (size_t playerId = 0; playerId < playerSystems_.size(); ++playerId) {
    int systemId = playerSystems_[playerId];
    if (systemId >= 0 && systemId < static_cast<int>(systems_.size())) {
      if (systems_[systemId].destroyed) {
        players_[playerId].alive = false;
      }
    }
  }

  for (auto& player : players_) {
    if (!player.alive) {
      continue;
    }
    if (player.nearDeath) {
      bool revived = false;
      if (survivalConfig_.enableRevival && !survivalConfig_.revivalBuildingCardIds.empty()) {
        auto playerId = static_cast<int>(&player - &players_.front());
        if (playerId >= 0 && playerId < static_cast<int>(playerBuildings_.size())) {
          auto& buildings = playerBuildings_[playerId];
          for (auto it = buildings.begin(); it != buildings.end(); ++it) {
            if (std::find(survivalConfig_.revivalBuildingCardIds.begin(),
                          survivalConfig_.revivalBuildingCardIds.end(),
                          *it) != survivalConfig_.revivalBuildingCardIds.end()) {
              buildings.erase(it);
              revived = true;
              break;
            }
          }
        }
      }

      if (player.defenseLevel > 0 || revived) {
        player.nearDeath = false;
      } else {
        player.alive = false;
      }
    }
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
