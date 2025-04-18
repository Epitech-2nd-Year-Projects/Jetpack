#pragma once

#include "../Shared/Protocol.hpp"
#include <string>
#include <unistd.h>
#include <atomic>
#include <memory>
#include <thread>

#include "GameDisplay.hpp"

namespace Jetpack::Client {
class NetworkClient {
public:
  explicit NetworkClient(int serverPort = 8080, std::string serverAddress = "", bool debugMode = false);
  ~NetworkClient();

  bool connectToServer();
  void start();

private:
  void networkLoop();

  void processPacket(const uint8_t *data, size_t length);
  void handleConnectResponse(const uint8_t *data, size_t length);
  void handleMapData(const uint8_t *data, size_t length);
  void handleGameStart(const uint8_t *data, size_t length);
  void handleGameStateUpdate(const uint8_t *data, size_t length);
  void handleCoinCollected(const uint8_t *data, size_t length) const;
  void handlePlayerDeath(const uint8_t *data, size_t length) const;
  void handleGameOver(const uint8_t *data, size_t length) const;

  void sendPlayerInput() const;

  int m_serverPort;
  std::string m_serverAddress;
  bool m_debugMode = false;
  int m_serverSocket = -1;
  int m_localPlayerId = -1;

  Shared::Protocol::GameMap m_map;
  std::vector<Shared::Protocol::Player> m_players;

  std::atomic<bool> m_running;
  std::thread m_networkThread;

  std::shared_ptr<GameDisplay> m_display;
};
} // namespace Jetpack::Client
