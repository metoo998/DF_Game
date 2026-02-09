#include "Roles.h"

RoleCatalog::RoleCatalog() {
  roles_ = {
      {RoleType::Human,
       "Human",
       {{"Interstellar Migration",
         "Near-death: spend all energy to move to a random intact system and mark it as a colony; usable once per game; Time Interference resets usage."}}},
      {RoleType::Observer,
       "Observer",
       {{"Observation",
         "Every 2 rounds, detect if a system has a player. If empty, build a Listening Station to enable broadcast/strike intel ignoring distance; link is established immediately and active next round."}}},
      {RoleType::Trisolarian,
       "Trisolarian",
       {{"Tech Lockdown",
         "Spend 4 energy to discard all building cards in target system; projectile speed 1; starts with one use then gains one use every 5 rounds (no stacking)."}}},
      {RoleType::Conquer,
       "Conquer",
       {{"Conquest",
         "Every 5 rounds, spend energy to launch Interstellar Expedition without a card; expedition speed 1."}}},
      {RoleType::Evaluator,
       "Evaluator",
       {{"Evolution",
         "After destroying a faction or challenging via Interstellar Expedition (success or failure), gain that faction's skill."}}},
  };
}
