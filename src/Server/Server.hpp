#pragma once

#include "../Shared/Protocol.hpp"
#include "Broadcaster.hpp"
#include <filesystem>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <unordered_map>

namespace Jetpack::Server {
class GameServer {
public:
  GameServer(int port, const std::string &mapFile, bool debugMode = false);
  ~GameServer();

  void start();

private:
  static constexpr int MAX_CLIENTS = 2;
  static constexpr int MIN_PLAYERS = 2;
  static constexpr int GAME_TICK_MS = 16;
  static constexpr int BUFFER_SIZE = 1024;

  bool loadMap();
  void initializeSocket();

  void handleSocketEvents();
  void acceptNewClient();
  void handleClientData(int clientSocket);
  void handleClientDisconnect(int clientSocket);

  void sendConnectResponse(int clientSocket, int playerId);
  void sendMapData(int clientSocket);

  void checkGameStart();

  void updateGameState();
  void updatePlayers();
  void checkCollisions();
  void checkGameEnd();

  void processPacket(int clientSocket, const uint8_t *data, size_t length);
  void handlePlayerInput(int clientSocket, const uint8_t *data, size_t length);

private:
  int m_port;
  std::filesystem::path m_mapFile;
  bool m_debugMode;

  Shared::Protocol::GameMap m_map;

  int m_serverSocket = -1;

  std::vector<pollfd> m_pollfds;

  std::unordered_map<int, Shared::Protocol::Player> m_players;

  Broadcaster m_broadcaster;

  Shared::Protocol::GameState m_gameState =
      Shared::Protocol::GameState::WAITING_FOR_PLAYERS;

  bool m_running = true;
};
} // namespace Jetpack::Server
