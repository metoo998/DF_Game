#pragma once

#include <string>
#include <vector>

enum class RoleType {
  Human,
  Observer,
  Trisolarian,
  Conquer,
  Evaluator
};

struct RoleAbility {
  std::string name;
  std::string description;
};

struct RoleDefinition {
  RoleType type;
  std::string title;
  std::vector<RoleAbility> abilities;
};

class RoleCatalog {
public:
  RoleCatalog();
  const std::vector<RoleDefinition>& all() const { return roles_; }

private:
  std::vector<RoleDefinition> roles_{};
};
