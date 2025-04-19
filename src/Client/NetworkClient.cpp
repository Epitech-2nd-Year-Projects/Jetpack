#include "NetworkClient.hpp"
#include "../Shared/Exceptions.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

Jetpack::Client::NetworkClient::NetworkClient(const int serverPort,
                                              std::string serverAddress,
                                              const bool debugMode)
    : m_serverPort(serverPort), m_serverAddress(std::move(serverAddress)),
      m_debugMode(debugMode) {}

Jetpack::Client::NetworkClient::~NetworkClient() {
  m_running = false;
  if (m_networkThread.joinable()) {
    m_networkThread.join();
  }

  if (m_serverSocket != -1) {
    ::close(m_serverSocket);
  }
}

bool Jetpack::Client::NetworkClient::connectToServer() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to create socket");
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(m_serverPort);

  if (inet_pton(AF_INET, m_serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
    ::close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Invalid server address");
  }

  if (::connect(m_serverSocket,
                reinterpret_cast<struct sockaddr *>(&serverAddr),
                sizeof(serverAddr)) < 0) {
    ::close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to connect to server");
  }

  int flags = fcntl(m_serverSocket, F_GETFL, 0);
  fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

  uint8_t buffer[2];
  buffer[0] =
      static_cast<uint8_t>(Shared::Protocol::PacketType::CONNECT_REQUEST);
  buffer[1] = 0;

  if (::send(m_serverSocket, buffer, sizeof(buffer), 0) != sizeof(buffer)) {
    std::cerr << "Failed to send connection request" << std::endl;
    close(m_serverSocket);
    m_serverSocket = -1;
    return false;
  }

  return true;
}

void Jetpack::Client::NetworkClient::start() {
  m_display = std::make_shared<GameDisplay>();
  m_networkThread = std::thread(&NetworkClient::networkLoop, this);

  while (m_localPlayerId == -1 && m_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (m_localPlayerId != -1) {
    m_display->setLocalPlayerId(m_localPlayerId);
  }
  m_display->run();
  m_running = false;
  if (m_networkThread.joinable()) {
    m_networkThread.join();
  }
}

void Jetpack::Client::NetworkClient::networkLoop() {
  constexpr int BUFFER_SIZE = 1024;
  uint8_t recvBuffer[BUFFER_SIZE];

  const auto inputUpdateInterval = std::chrono::milliseconds(16);
  auto lastInputUpdate = std::chrono::steady_clock::now();

  while (m_running) {
    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime - lastInputUpdate >= inputUpdateInterval) {
      sendPlayerInput();
      lastInputUpdate = currentTime;
    }

    ssize_t bytesRead = recv(m_serverSocket, recvBuffer, BUFFER_SIZE, 0);
    if (bytesRead > 0) {
      if (m_debugMode) {
        std::cout << "Received " << bytesRead << " bytes: ";
        for (ssize_t i = 0; i < bytesRead; i++) {
          std::cout << std::hex << static_cast<int>(recvBuffer[i]) << " ";
        }
        std::cout << std::dec << std::endl;
      }

      size_t processedBytes = 0;
      while (processedBytes < static_cast<size_t>(bytesRead)) {
        size_t packetSize = getPacketSize(recvBuffer + processedBytes,
                                          bytesRead - processedBytes);
        if (packetSize == 0) {
          break;
        }
        processPacket(recvBuffer + processedBytes, packetSize);
        processedBytes += packetSize;
      }
    } else if (bytesRead < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Error reading from server: " << strerror(errno)
                  << std::endl;
        break;
      }
    }
  }
}

size_t Jetpack::Client::NetworkClient::getPacketSize(const uint8_t *data,
                                                     size_t maxSize) const {
  if (maxSize < 1) {
    return 0;
  }

  switch (Shared::Protocol::PacketType packetType =
              static_cast<Shared::Protocol::PacketType>(data[0])) {
  case Shared::Protocol::PacketType::CONNECT_RESPONSE:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::MAP_DATA: {
    if (maxSize < 5) {
      return 0;
    }
    const int width = data[1] | (data[2] << 8);
    const int height = data[3] | (data[4] << 8);
    const size_t expectedSize = 5 + width * height;
    return (maxSize >= expectedSize) ? expectedSize : 0;
  }

  case Shared::Protocol::PacketType::GAME_START:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::GAME_STATE_UPDATE: {
    if (maxSize < 2) {
      return 0;
    }
    int playerCount = data[1];
    constexpr size_t playerDataSize = 10;
    size_t expectedSize = 2 + playerCount * playerDataSize;
    return (maxSize >= expectedSize) ? expectedSize : 0;
  }

  case Shared::Protocol::PacketType::COIN_COLLECTED:
    return (maxSize >= 5) ? 5 : 0;

  case Shared::Protocol::PacketType::PLAYER_DEATH:
    return (maxSize >= 2) ? 2 : 0;

  case Shared::Protocol::PacketType::GAME_OVER:
    return (maxSize >= 3) ? 3 : 0;

  case Shared::Protocol::PacketType::PLAYER_INPUT:
    return (maxSize >= 2) ? 2 : 0;

  default:
    if (m_debugMode) {
      std::cout << "Unknown packet type: " << static_cast<int>(packetType)
                << std::endl;
    }
    return 0;
  }
}

void Jetpack::Client::NetworkClient::processPacket(const uint8_t *data,
                                                   size_t length) {
  if (length < 1) {
    return;
  }

  auto packetType = static_cast<Shared::Protocol::PacketType>(data[0]);
  if (m_debugMode) {
    std::cout << "Processing packet type: " << static_cast<int>(packetType)
              << std::endl;
  }
  switch (auto packetType =
              static_cast<Shared::Protocol::PacketType>(data[0])) {
  case Shared::Protocol::PacketType::CONNECT_RESPONSE:
    handleConnectResponse(data, length);
    break;
  case Shared::Protocol::PacketType::MAP_DATA:
    handleMapData(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_START:
    handleGameStart(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_STATE_UPDATE:
    handleGameStateUpdate(data, length);
    break;
  case Shared::Protocol::PacketType::COIN_COLLECTED:
    handleCoinCollected(data, length);
    break;
  case Shared::Protocol::PacketType::PLAYER_DEATH:
    handlePlayerDeath(data, length);
    break;
  case Shared::Protocol::PacketType::GAME_OVER:
    handleGameOver(data, length);
    break;
  default:
    if (m_debugMode) {
      std::cout << "Unknown packet type: " << static_cast<int>(packetType)
                << std::endl;
    }
    break;
  }
}

void Jetpack::Client::NetworkClient::handleConnectResponse(const uint8_t *data,
                                                           size_t length) {
  if (length < 3) {
    return;
  }

  m_localPlayerId = data[1];
  const int totalPlayers = data[2];
  m_running = true;

  if (m_debugMode) {
    std::cout << "Connected as player " << m_localPlayerId
              << ", total players: " << totalPlayers << std::endl;
  }
}

void Jetpack::Client::NetworkClient::handleMapData(const uint8_t *data,
                                                   const size_t length) {
  if (length < 5) {
    return;
  }

  const int width = data[1] | (data[2] << 8);
  const int height = data[3] | (data[4] << 8);

  if (length < static_cast<size_t>(5) + width * height) {
    return;
  }

  m_map.width = width;
  m_map.height = height;
  m_map.tiles.resize(height, std::vector<Shared::Protocol::TileType>(width));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      m_map.tiles[y][x] =
          static_cast<Shared::Protocol::TileType>(data[5 + y * width + x]);
    }
  }

  if (m_debugMode) {
    std::cout << "Received map data: " << width << "x" << height << std::endl;
  }

  if (m_display) {
    m_display->updateMap(m_map);
  }
}

void Jetpack::Client::NetworkClient::handleGameStart(const uint8_t *data,
                                                     const size_t length) {
  if (length < 3) {
    return;
  }

  int playerCount = data[1];

  if (m_debugMode) {
    std::cout << "Game starting with " << playerCount << " players"
              << std::endl;
  }

  m_players.clear();
  if (m_players.empty()) {
    for (int i = 1; i <= playerCount; i++) {
      m_players.emplace_back(-1, i);
      m_players.back().setState(Shared::Protocol::PlayerState::PLAYING);
    }
    if (m_display) {
      m_display->updateGameState(m_players);
    }
  }
}

void Jetpack::Client::NetworkClient::handleGameStateUpdate(
    const uint8_t *data, const size_t length) {
  if (length < 2) {
    return;
  }

  const int playerCount = data[1];
  constexpr size_t playerDataSize = 10;

  if (length < 2 + playerCount * playerDataSize) {
    return;
  }

  for (int i = 0; i < playerCount; i++) {
    const int offset = 2 + i * playerDataSize;
    const int playerId = data[offset];
    const auto state =
        static_cast<Shared::Protocol::PlayerState>(data[offset + 1]);

    const int16_t xFixedPrecision = data[offset + 2] | (data[offset + 3] << 8);
    const int16_t yFixedPrecision = data[offset + 4] | (data[offset + 5] << 8);
    const float x = xFixedPrecision / 100.0f;
    const float y = yFixedPrecision / 100.0f;

    const int score = data[offset + 6] | (data[offset + 7] << 8);
    bool isJetpacking = data[offset + 8] != 0;

    bool found = false;
    for (auto &player : m_players) {
      if (player.getId() == playerId) {
        player.setState(state);
        player.setPosition(x, y);
        player.setScore(score);
        player.setJetpacking(isJetpacking);
        found = true;
        break;
      }
    }

    if (!found) {
      m_players.emplace_back(-1, playerId);
      m_players.back().setState(state);
      m_players.back().setPosition(x, y);
      m_players.back().setScore(score);
      m_players.back().setJetpacking(isJetpacking);
    }
  }

  if (m_display) {
    m_display->updateGameState(m_players);
  }
}

void Jetpack::Client::NetworkClient::handleCoinCollected(
    const uint8_t *data, const size_t length) const {
  if (length < 5) {
    return;
  }

  const int playerId = data[1];
  const int x = data[2];
  const int y = data[3];
  const int newScore = data[4];

  if (m_debugMode) {
    std::cout << "Player " << playerId << " collected coin at (" << x << ","
              << y << "), new score: " << newScore << std::endl;
  }

  if (m_display) {
    m_display->handleCoinCollected(playerId, x, y);
  }
}

void Jetpack::Client::NetworkClient::handlePlayerDeath(
    const uint8_t *data, const size_t length) const {
  if (length < 2) {
    return;
  }

  int playerId = data[1];

  if (m_debugMode) {
    std::cout << "Player " << playerId << " died" << std::endl;
  }

  if (m_display) {
    m_display->handlePlayerDeath(playerId);
  }
}

void Jetpack::Client::NetworkClient::handleGameOver(const uint8_t *data,
                                                    const size_t length) const {
  if (length < 3) {
    return;
  }

  bool hasWinner = data[1] != 0;
  int winnerId = hasWinner ? data[2] : -1;

  if (m_debugMode) {
    if (hasWinner) {
      std::cout << "Game over! Player " << winnerId << " wins!" << std::endl;
    } else {
      std::cout << "Game over! No winner." << std::endl;
    }
  }

  if (m_display) {
    m_display->handleGameOver(winnerId);
  }
}

void Jetpack::Client::NetworkClient::sendPlayerInput() const {
  if (m_serverSocket < 0 || !m_display) {
    return;
  }

  bool jetpackActive = m_display->isJetpackActive();

  uint8_t buffer[2];
  buffer[0] = static_cast<uint8_t>(Shared::Protocol::PacketType::PLAYER_INPUT);
  buffer[1] = jetpackActive ? 1 : 0;

  send(m_serverSocket, buffer, sizeof(buffer), 0);
}

int Jetpack::Client::NetworkClient::getLocalPlayerId() const {
  return m_localPlayerId;
}
