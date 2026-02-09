#pragma once

#include <string>
#include <vector>

struct CardDefinition;

enum class Phase {
  Preparation,
  Play,
  Resolution
};

enum class EffectType {
  TypeI,
  TypeII,
  TypeIII
};

struct Projectile {
  std::string cardName;
  int remainingTime = 1;
  int ownerId = -1;
  int targetSystemId = -1;
  int level = 0;
  int cardId = 0;
};

struct PlayerParams {
  int energy = 3;
  int draw = 0;
  int cooldown = 0;
  bool alive = true;
  int defenseLevel = 0;
  std::string pendingBroadcastChoice = "";
};

struct SystemState {
  bool starExists = true;
  bool isDimensionalized = false;
  bool destroyed = false;
  bool occupied = false;
  bool colonized = false;
  std::vector<int> neighbors{};
};

struct ObservedSystemState {
  int systemId = -1;
  std::string starStatus;
  int ageInRounds = 0;
};

struct WarningReport {
  int systemId = -1;
  std::string message;
};

struct PhaseReport {
  Phase phase;
  std::string summary;
};

struct ObservationReport {
  std::vector<ObservedSystemState> systems;
  std::vector<WarningReport> warnings;
};

class GameRules {
public:
  PhaseReport runPreparation(int playerId);
  PhaseReport runPlay(int playerId);
  PhaseReport runResolution(int playerId);

  ObservationReport generateObservations(int playerId);

  void addSystem(int systemId);
  const SystemState& system(int systemId) const;
  void setSystemNeighbors(int systemId, const std::vector<int>& neighbors);

  bool resolveCardPlay(const CardDefinition& card, int playerId, int targetSystemId,
                       bool targetHasCivilization);

  void queueProjectile(const std::string& cardName, int ownerId, int targetSystemId, int level,
                       int cardId);
  void applyTypeIIEffect(const std::string& cardName, int ownerId);
  void applyTypeIIIEffect(const std::string& cardName, int ownerId);
  void applyTimeInterference(int systemId, int ownerId, bool targetHasCivilization);
  void applyTechLockdown(int systemId, int ownerId);
  void applyInterstellarExpedition(int systemId, int ownerId);

  void addPlayer(int playerId);
  void setBroadcastResponseChoice(int playerId, const std::string& choice);
  void setPlayerSystem(int playerId, int systemId);
  int playerSystem(int playerId) const;
  const std::vector<Projectile>& projectiles() const { return projectiles_; }
  const std::vector<PlayerParams>& players() const { return players_; }

private:
  void updateProjectiles();
  void resolveTypeI();
  void resolveStrikeProjectile(const Projectile& projectile);
  void resolveBroadcastProjectile(const Projectile& projectile);
  bool playerHasListeningBase(int playerId) const;
  std::string broadcastVariant(const Projectile& projectile) const;
  bool isSystemWithinDistance(int startSystemId, int targetSystemId, int distance) const;
  void updateTypeIII();
  void updatePlayerParams(int playerId);
  bool spendEnergy(int playerId, int cost);
  void addBuilding(int playerId, int cardId);

  std::vector<Projectile> projectiles_{};
  std::vector<PlayerParams> players_{};
  std::vector<int> playerSystems_{};
  std::vector<SystemState> systems_{};
  std::vector<std::vector<int>> playerBuildings_{};
};
