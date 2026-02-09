#pragma once

#include "GameRules.h"
#include <string>
#include <vector>

enum class CardCategory {
  Broadcast,
  Energy,
  Defense,
  Strike,
  Special,
  Building,
  Skill
};

struct CardDefinition {
  int id = 0;
  std::string title;
  int cost = 0;
  CardCategory category = CardCategory::Special;
  EffectType effectType = EffectType::TypeI;
  std::string variant;
  std::string type;
  std::string effect;
  std::string description;
  std::string property;
  int broadcastRange = -1;
  int level = 0;
};

class CardCatalog {
public:
  CardCatalog();

  const std::vector<CardDefinition>& all() const { return cards_; }
  const CardDefinition* findById(int id) const;

private:
  std::vector<CardDefinition> cards_{};
};
