#include "Assets.h"

#include <iostream>

void Assets::load() {
  // TODO: Load actual assets here. The placeholders describe where images will be shown.
  placeholders_.clear();
  placeholders_.push_back({"Background", "assets/backgrounds/game_board.png"});
  placeholders_.push_back({"Player Hand", "assets/cards/player_hand/*.png"});
  placeholders_.push_back({"Opponent Hand", "assets/cards/opponent_hand/*.png"});
  placeholders_.push_back({"Battlefield", "assets/cards/battlefield/*.png"});

  std::cout << "[Assets] Using fixed asset paths under ./assets/" << "\n";
}
