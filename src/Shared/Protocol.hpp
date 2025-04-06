#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace Jetpack::Shared::Protocol {
enum class TileType : uint8_t {
  Empty = 0,
  Wall = 1,
  Coin = 2,
  ElectricSquare = 3,
  EndPoint = 4
};

struct Position {
  float x;
  float y;

  bool operator==(const Position &other) const {
    return x == other.x && y == other.y;
  }
};

struct GameMap {
  int width = 0;
  int height = 0;
  std::vector<std::vector<TileType>> tiles;
  std::vector<Position> coinPositions;
};

enum class MessageType : uint8_t {
  Connect = 1,
  Disconnect = 2,
  GameStart = 3,
  MapData = 4,
  PlayerUpdate = 5,
  GameEvent = 6,
  GameEnd = 7
};

enum class GameEventType { PlayerDeath = 1, CoinCollected = 2 };
} // namespace Jetpack::Shared::Protocol
