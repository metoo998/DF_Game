#include "AiClient.h"

#include <iostream>

AiClient::AiClient(std::string modelPath) : modelPath_(std::move(modelPath)) {}

void AiClient::loadModel() {
  // TODO: Load AI model from host machine storage.
  std::cout << "[AI] Loading model from: " << modelPath_ << "\n";
}

void AiClient::evaluateTurn() {
  // TODO: Evaluate turn using AI model.
  std::cout << "[AI] Evaluating turn using AI model.\n";
}
