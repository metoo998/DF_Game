#pragma once

#include <string>

class AiClient {
public:
  explicit AiClient(std::string modelPath);

  void loadModel();
  void evaluateTurn();

private:
  std::string modelPath_;
};
