#include "Server.hpp"
#include "../Shared/Exceptions.hpp"
#include "Physics.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <vector>

Jetpack::Server::GameServer::GameServer(int port, const std::string &mapFile,
                                        bool debugMode)
    : m_port(port), m_mapFile(mapFile), m_debugMode(debugMode),
      m_broadcaster(m_players, m_debugMode) {
  if (!loadMap()) {
    throw Jetpack::Shared::Exceptions::MapLoaderException(
        "Failed to load map file: " + m_mapFile.string());
  }
  initializeSocket();
}

Jetpack::Server::GameServer::~GameServer() {
  for (const auto &playerPair : m_players) {
    close(playerPair.first);
  }
  close(m_serverSocket);
}

void Jetpack::Server::GameServer::start() {
  while (m_running) {
    int ready = poll(m_pollfds.data(), m_pollfds.size(), GAME_TICK_MS);

    if (ready < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    handleSocketEvents();
    updateGameState();
  }
}

bool Jetpack::Server::GameServer::loadMap() {
  std::ifstream file(m_mapFile);
  if (!file.is_open()) {
    return false;
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }

  if (lines.empty()) {
    return false;
  }

  m_map.height = lines.size();
  m_map.width = lines[0].length();

  for (const auto &line : lines) {
    if (line.length() != static_cast<size_t>(m_map.width)) {
      return false;
    }
  }

  m_map.tiles.resize(m_map.height,
                     std::vector<Shared::Protocol::TileType>(
                         m_map.width, Shared::Protocol::TileType::EMPTY));

  for (int y = 0; y < m_map.height; y++) {
    for (int x = 0; x < m_map.width; x++) {
      switch (lines[y][x]) {
      case '_':
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
        break;
      case 'c':
        m_map.tiles[y][x] = Shared::Protocol::TileType::COIN;
        break;
      case 'e':
        m_map.tiles[y][x] = Shared::Protocol::TileType::ELECTRICSQUARE;
        break;
      default:
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
        break;
      }
    }
  }

  return true;
}

void Jetpack::Server::GameServer::initializeSocket() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to create socket");
  }

  int flags = fcntl(m_serverSocket, F_GETFL, 0);
  fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

  int opt = 1;
  setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(m_port);

  if (bind(m_serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException("Failed to bind socket");
  }

  if (listen(m_serverSocket, MAX_CLIENTS) < 0) {
    close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to listen on socket");
  }

  pollfd pfd = {m_serverSocket, POLLIN, 0};
  m_pollfds.push_back(pfd);
}

void Jetpack::Server::GameServer::handleSocketEvents() {
  for (size_t i = 0; i < m_pollfds.size(); i++) {
    if (m_pollfds[i].revents & POLLIN) {
      if (m_pollfds[i].fd == m_serverSocket) {
        acceptNewClient();
      } else {
        handleClientData(m_pollfds[i].fd);
      }
    } else if (m_pollfds[i].revents & (POLLHUP | POLLERR)) {
      if (m_pollfds[i].fd != m_serverSocket) {
        handleClientDisconnect(m_pollfds[i].fd);
        i--;
      }
    }
  }
}

void Jetpack::Server::GameServer::acceptNewClient() {
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);

  int clientSocket =
      accept(m_serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientSocket < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
    }
    return;
  }

  int flags = fcntl(clientSocket, F_GETFL, 0);
  fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

  pollfd pfd = {clientSocket, POLLIN, 0};
  m_pollfds.push_back(pfd);

  int newPlayerId = m_players.size() + 1;
  m_players.emplace(clientSocket,
                    Shared::Protocol::Player(clientSocket, newPlayerId));

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, client_ip, INET_ADDRSTRLEN);

  sendConnectResponse(clientSocket, newPlayerId);
  sendMapData(clientSocket);

  checkGameStart();
}

void Jetpack::Server::GameServer::handleClientDisconnect(int clientSocket) {
  auto it = m_players.find(clientSocket);
  if (it != m_players.end()) {
    m_players.erase(it);
  }

  for (size_t i = 0; i < m_pollfds.size(); i++) {
    if (m_pollfds[i].fd == clientSocket) {
      close(clientSocket);
      m_pollfds.erase(m_pollfds.begin() + i);
      break;
    }
  }

  if (m_gameState == Shared::Protocol::GameState::IN_PROGRESS) {
    int activePlayers = 0;
    for (const auto &playerPair : m_players) {
      if (playerPair.second.getState() ==
          Shared::Protocol::PlayerState::PLAYING) {
        activePlayers++;
      }
    }

    if (activePlayers < MIN_PLAYERS) {
      m_gameState = Shared::Protocol::GameState::GAME_OVER;
      m_broadcaster.broadcastGameOver();
    }
  }
}

void Jetpack::Server::GameServer::handleClientData(int clientSocket) {
  uint8_t buffer[BUFFER_SIZE];
  ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);

  if (bytesRead <= 0) {
    if (bytesRead == 0 ||
        (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      handleClientDisconnect(clientSocket);
    }
    return;
  }

  if (m_debugMode) {
    std ::cout << std::format(
        "Debug: Received {} bytes from client {} - Buffer: ", bytesRead,
        clientSocket);
    for (ssize_t i = 0; i < bytesRead; i++) {
      std::cout << std::format("{:02X} ", buffer[i]);
    }
    std::cout << std::endl;
  }

  processPacket(clientSocket, buffer, bytesRead);
}

void Jetpack::Server::GameServer::processPacket(int clientSocket,
                                                const uint8_t *data,
                                                size_t length) {
  if (length < 1)
    return;

  Shared::Protocol::PacketType type =
      static_cast<Shared::Protocol::PacketType>(data[0]);

  switch (type) {
  case Shared::Protocol::PacketType::CONNECT_REQUEST:
    break;
  case Shared::Protocol::PacketType::PLAYER_INPUT:
    handlePlayerInput(clientSocket, data, length);
    break;
  case Shared::Protocol::PacketType::PLAYER_DISCONNECT:
    handleClientDisconnect(clientSocket);
    break;
  default:
    break;
  }
}

void Jetpack::Server::GameServer::handlePlayerInput(int clientSocket,
                                                    const uint8_t *data,
                                                    size_t length) {
  if (length < 2)
    return;

  bool isJetpacking = data[1] != 0;

  auto it = m_players.find(clientSocket);
  if (it != m_players.end() &&
      it->second.getState() == Shared::Protocol::PlayerState::PLAYING) {
    it->second.setJetpacking(isJetpacking);
  }
}

void Jetpack::Server::GameServer::sendConnectResponse(int clientSocket,
                                                      int playerId) {
  uint8_t buffer[3];
  buffer[0] =
      static_cast<uint8_t>(Shared::Protocol::PacketType::CONNECT_RESPONSE);
  buffer[1] = playerId;
  buffer[2] = m_players.size();

  send(clientSocket, buffer, sizeof(buffer), 0);

  if (m_debugMode) {
    std::cout << std::format("Debug: Sent connection response to client {} "
                             "(Player ID: {}) - Buffer: ",
                             clientSocket, playerId);
    for (size_t i = 0; i < sizeof(buffer); i++) {
      std::cout << std::format("{:02X} ", buffer[i]);
    }
    std::cout << std::endl;
  }
}

void Jetpack::Server::GameServer::sendMapData(int clientSocket) {
  std::size_t bufferSize = 1 + 2 + 2 + m_map.width * m_map.height;
  std::vector<uint8_t> buffer(bufferSize);

  buffer[0] = static_cast<uint8_t>(Shared::Protocol::PacketType::MAP_DATA);

  buffer[1] = m_map.width & 0xFF;
  buffer[2] = (m_map.width >> 8) & 0xFF;
  buffer[3] = m_map.height & 0xFF;
  buffer[4] = (m_map.height >> 8) & 0xFF;

  for (int y = 0; y < m_map.height; y++) {
    for (int x = 0; x < m_map.width; x++) {
      buffer[5 + y * m_map.width + x] = static_cast<uint8_t>(m_map.tiles[y][x]);
    }
  }

  send(clientSocket, buffer.data(), buffer.size(), 0);

  if (m_debugMode) {
    std::cout << std::format(
        "Debug: Sent map data to client {} (Socket: {}) - Buffer: ",
        clientSocket, clientSocket);
    for (size_t i = 0; i < buffer.size(); i++) {
      std::cout << std::format("{:02X} ", buffer[i]);
    }
    std::cout << std::endl;
  }
}

void Jetpack::Server::GameServer::checkGameStart() {
  if (m_gameState != Shared::Protocol::GameState::WAITING_FOR_PLAYERS) {
    return;
  }

  int readyPlayersCount = m_players.size();

  if (readyPlayersCount >= MIN_PLAYERS) {
    m_gameState = Shared::Protocol::GameState::IN_PROGRESS;

    for (auto &playerPair : m_players) {
      playerPair.second.setState(Shared::Protocol::PlayerState::READY);

      constexpr float startX = 1.0f;
      const float startY = m_map.height / 2.0f;

      playerPair.second.setPosition(startX, startY);
      playerPair.second.setVelocityY(0.0f);
      playerPair.second.setScore(0);
    }

    m_broadcaster.broadcastGameStart();
    m_broadcaster.broadcastGameState();
  }
}

void Jetpack::Server::GameServer::updateGameState() {
  if (m_gameState != Shared::Protocol::GameState::IN_PROGRESS) {
    return;
  }

  bool allReady = true;
  bool anyPlaying = false;

  for (auto &playerPair : m_players) {
    if (playerPair.second.getState() ==
        Shared::Protocol::PlayerState::PLAYING) {
      anyPlaying = true;
    } else if (playerPair.second.getState() ==
               Shared::Protocol::PlayerState::READY) {
    } else {
      allReady = false;
    }
  }

  if (allReady && !anyPlaying) {
    for (auto &playerPair : m_players) {
      if (playerPair.second.getState() ==
          Shared::Protocol::PlayerState::READY) {
        playerPair.second.setState(Shared::Protocol::PlayerState::PLAYING);
      }
    }
    m_broadcaster.broadcastGameState();
    return;
  }

  updatePlayers();
  checkCollisions();
  m_broadcaster.broadcastGameState();
  checkGameEnd();
}

void Jetpack::Server::GameServer::updatePlayers() {
  for (auto &playerPair : m_players) {
    Shared::Protocol::Player &player = playerPair.second;

    if (player.getState() != Shared::Protocol::PlayerState::PLAYING) {
      continue;
    }

    Physics::applyPhysics(player);
    Physics::checkBounds(player, m_map);

    if (player.getPosition().x >= m_map.width) {
      player.setState(Shared::Protocol::PlayerState::FINISHED);
    }
  }
}

void Jetpack::Server::GameServer::checkCollisions() {
  for (auto &playerPair : m_players) {
    Shared::Protocol::Player &player = playerPair.second;

    if (player.getState() != Shared::Protocol::PlayerState::PLAYING) {
      continue;
    }

    int cell_x = static_cast<int>(player.getPosition().x);
    int cell_y = static_cast<int>(player.getPosition().y);

    if (cell_x >= 0 && cell_x < m_map.width && cell_y >= 0 &&
        cell_y < m_map.height) {
      Shared::Protocol::TileType tile = m_map.tiles[cell_y][cell_x];

      if (tile == Shared::Protocol::TileType::COIN) {
        player.setScore(player.getScore() + 1);
        m_map.tiles[cell_y][cell_x] = Shared::Protocol::TileType::EMPTY;

        m_broadcaster.broadcastCoinCollected(player.getId(), cell_x, cell_y);

      } else if (tile == Shared::Protocol::TileType::ELECTRICSQUARE) {
        player.setState(Shared::Protocol::PlayerState::DEAD);

        m_broadcaster.broadcastPlayerDeath(player.getId());
      }
    }
  }
}

void Jetpack::Server::GameServer::checkGameEnd() {
  bool allFinished = true;
  bool anyDead = false;
  int activePlayersCount = 0;

  for (const auto &playerPair : m_players) {
    const Shared::Protocol::Player &player = playerPair.second;

    if (player.getState() == Shared::Protocol::PlayerState::PLAYING) {
      allFinished = false;
      activePlayersCount++;
    } else if (player.getState() == Shared::Protocol::PlayerState::FINISHED) {
      activePlayersCount++;
    } else if (player.getState() == Shared::Protocol::PlayerState::DEAD) {
      anyDead = true;
    }
  }

  if ((allFinished && activePlayersCount > 0) || anyDead ||
      (activePlayersCount < MIN_PLAYERS && m_players.size() >= MIN_PLAYERS)) {
    m_gameState = Shared::Protocol::GameState::GAME_OVER;

    int winnerId = -1;
    int highestScore = -1;

    for (const auto &playerPair : m_players) {
      const Shared::Protocol::Player &player = playerPair.second;

      if (anyDead && player.getState() != Shared::Protocol::PlayerState::DEAD) {
        winnerId = player.getId();
        break;
      }

      if (player.getScore() > highestScore) {
        highestScore = player.getScore();
        winnerId = player.getId();
      }
    }

    m_broadcaster.broadcastGameOver(winnerId);
  }
}
