#include "Broadcaster.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <sys/socket.h>

void Jetpack::Server::Broadcaster::broadcastGameOver(int winnerId) {
  uint8_t buffer[3];
  buffer[0] = static_cast<uint8_t>(Shared::Protocol::PacketType::GAME_OVER);
  buffer[1] = (winnerId > 0) ? 1 : 0;
  buffer[2] = (winnerId > 0) ? winnerId : 0;

  for (const auto &playerPair : m_serverPlayersReference) {
    send(playerPair.first, buffer, sizeof(buffer), 0);
    if (m_debugMode) {
      std::cout << std::format(
          "Debug: Sent game over broadcast to client {} - Buffer: ",
          playerPair.first);
      for (size_t i = 0; i < sizeof(buffer); i++) {
        std::cout << std::format("{:02X} ", buffer[i]);
      }
      std::cout << std::endl;
    }
  }
}

void Jetpack::Server::Broadcaster::broadcastPlayerDeath(int playerId) {
  uint8_t buffer[2];
  buffer[0] = static_cast<uint8_t>(Shared::Protocol::PacketType::PLAYER_DEATH);
  buffer[1] = playerId;

  for (const auto &playerPair : m_serverPlayersReference) {
    send(playerPair.first, buffer, sizeof(buffer), 0);
    if (m_debugMode) {
      std::cout << std::format(
          "Debug: Sent player death broadcast to client {} - Buffer: ",
          playerPair.first);
      for (size_t i = 0; i < sizeof(buffer); i++) {
        std::cout << std::format("{:02X} ", buffer[i]);
      }
      std::cout << std::endl;
    }
  }
}

void Jetpack::Server::Broadcaster::broadcastCoinCollected(int playerId, int x,
                                                          int y) {
  uint8_t buffer[5];
  buffer[0] =
      static_cast<uint8_t>(Shared::Protocol::PacketType::COIN_COLLECTED);
  buffer[1] = playerId;
  buffer[2] = x;
  buffer[3] = y;
  buffer[4] = m_serverPlayersReference
                  .at(std::find_if(m_serverPlayersReference.begin(),
                                   m_serverPlayersReference.end(),
                                   [playerId](const auto &p) {
                                     return p.second.getId() == playerId;
                                   })
                          ->first)
                  .getScore();

  for (const auto &playerPair : m_serverPlayersReference) {
    send(playerPair.first, buffer, sizeof(buffer), 0);
    if (m_debugMode) {
      std::cout << std::format(
          "Debug: Sent coin collected broadcast to client {} - Buffer: ",
          playerPair.first);
      for (size_t i = 0; i < sizeof(buffer); i++) {
        std::cout << std::format("{:02X} ", buffer[i]);
      }
      std::cout << std::endl;
    }
  }
}

void Jetpack::Server::Broadcaster::broadcastGameState() {
  size_t playerDataSize = 10;
  size_t bufferSize = 2 + (m_serverPlayersReference.size() * playerDataSize);
  std::vector<uint8_t> buffer(bufferSize);

  buffer[0] =
      static_cast<uint8_t>(Shared::Protocol::PacketType::GAME_STATE_UPDATE);
  buffer[1] = m_serverPlayersReference.size();

  size_t offset = 2;
  for (const auto &playerPair : m_serverPlayersReference) {
    const Shared::Protocol::Player &player = playerPair.second;

    buffer[offset] = player.getId();
    buffer[offset + 1] = static_cast<uint8_t>(player.getState());

    int16_t xFixedPrecision =
        static_cast<int16_t>(player.getPosition().x * 100);
    int16_t yFixedPrecision =
        static_cast<int16_t>(player.getPosition().y * 100);

    buffer[offset + 2] = xFixedPrecision & 0xFF;
    buffer[offset + 3] = (xFixedPrecision >> 8) & 0xFF;
    buffer[offset + 4] = yFixedPrecision & 0xFF;
    buffer[offset + 5] = (yFixedPrecision >> 8) & 0xFF;

    buffer[offset + 6] = player.getScore() & 0xFF;
    buffer[offset + 7] = (player.getScore() >> 8) & 0xFF;

    buffer[offset + 8] = player.isJetpacking() ? 1 : 0;

    buffer[offset + 9] = 0;

    offset += playerDataSize;
  }

  for (const auto &playerPair : m_serverPlayersReference) {
    send(playerPair.first, buffer.data(), buffer.size(), 0);
    if (m_debugMode) {
      std::cout << std::format(
          "Debug: Sent game state update broadcast to client {} - Buffer: ",
          playerPair.first);
      for (size_t i = 0; i < buffer.size(); i++) {
        std::cout << std::format("{:02X} ", buffer[i]);
      }
      std::cout << std::endl;
    }
  }
}

void Jetpack::Server::Broadcaster::broadcastGameStart() {
  uint8_t buffer[3];
  buffer[0] = static_cast<uint8_t>(Shared::Protocol::PacketType::GAME_START);
  buffer[1] = m_serverPlayersReference.size();
  buffer[2] = 0;

  for (const auto &playerPair : m_serverPlayersReference) {
    send(playerPair.first, buffer, sizeof(buffer), 0);
    if (m_debugMode) {
      std::cout << std::format(
          "Debug: Sent game start broadcast to client {} - Buffer: ",
          playerPair.first);
      for (size_t i = 0; i < sizeof(buffer); i++) {
        std::cout << std::format("{:02X} ", buffer[i]);
      }
      std::cout << std::endl;
    }
  }
}
