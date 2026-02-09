cmake_minimum_required(VERSION 3.20)
project(DF_Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(df_game
  src/main.cpp
  src/CLI.cpp
  src/Network.cpp
  src/AiClient.cpp
  src/Card.cpp
  src/Cards.cpp
  src/Roles.cpp
  src/Game.cpp
  src/GameRules.cpp
  src/Assets.cpp
)

target_include_directories(df_game PRIVATE src)
