#pragma once

#include "GameRules.h"
#include <string>

struct Card {
  int id{};
  std::string name;
  std::string description;
  EffectType effectType = EffectType::TypeI;

  // Placeholder for the card's gameplay effect.
  // Implement concrete logic here when updating card behavior.
  void applyEffect() const;
};
