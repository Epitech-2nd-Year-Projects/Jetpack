#pragma once

#include "../Shared/Protocol.hpp"
#include <unordered_map>

namespace Jetpack::Server {
class Broadcaster {
public:
  Broadcaster(
      std::unordered_map<int, Shared::Protocol::Player> &serverPlayersReference,
      bool debugMode = false)
      : m_serverPlayersReference(serverPlayersReference),
        m_debugMode(debugMode) {}

  void broadcastGameStart();
  void broadcastGameState();
  void broadcastCoinCollected(int playerId, int x, int y);
  void broadcastPlayerDeath(int playerId);
  void broadcastGameOver(int winnerId = -1);

private:
  std::unordered_map<int, Shared::Protocol::Player> &m_serverPlayersReference;
  bool m_debugMode = false;
};
} // namespace Jetpack::Server
