#pragma once

#include <string>
#include <vector>

struct AssetSlot {
  std::string label;
  std::string pathHint;
};

class Assets {
public:
  void load();
  const std::vector<AssetSlot>& placeholders() const { return placeholders_; }

private:
  std::vector<AssetSlot> placeholders_{};
};
