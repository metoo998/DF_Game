#pragma once

#include <string>
#include <vector>

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
};

struct PlayerParams {
  int energy = 3;
  int draw = 0;
  int cooldown = 0;
  bool alive = true;
};

struct SystemState {
  bool starExists = true;
  bool isDimensionalized = false;
  bool destroyed = false;
  bool occupied = false;
  bool colonized = false;
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

  void queueProjectile(const std::string& cardName, int ownerId);
  void applyTypeIIEffect(const std::string& cardName, int ownerId);
  void applyTypeIIIEffect(const std::string& cardName, int ownerId);
  void applyTimeInterference(int systemId, int ownerId, bool targetHasCivilization);
  void applyTechLockdown(int systemId, int ownerId);
  void applyInterstellarExpedition(int systemId, int ownerId);

  void addPlayer(int playerId);
  const std::vector<Projectile>& projectiles() const { return projectiles_; }
  const std::vector<PlayerParams>& players() const { return players_; }

private:
  void updateProjectiles();
  void resolveTypeI();
  void updateTypeIII();
  void updatePlayerParams(int playerId);

  std::vector<Projectile> projectiles_{};
  std::vector<PlayerParams> players_{};
  std::vector<SystemState> systems_{};
};
