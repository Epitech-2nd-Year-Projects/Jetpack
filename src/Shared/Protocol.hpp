#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace Jetpack::Shared::Protocol {
enum class TileType { EMPTY, COIN, ELECTRICSQUARE };

struct GameMap {
  int width = 0;
  int height = 0;
  std::vector<std::vector<TileType>> tiles;
};

struct Position {
  float x = 0;
  float y = 0;
};

enum class PlayerState {
  CONNECTED,
  READY,
  PLAYING,
  DEAD,
  FINISHED,
  DISCONNECTED,
};

class Player {
private:
  int m_clientSocket = -1;
  int m_id = -1;

  Position m_position = {0, 0};
  float m_velocityY = 0.0f;
  bool m_isJetpacking = false;

  int m_Score = 0;

  PlayerState m_state = PlayerState::CONNECTED;

public:
  Player(int clientSocket, int playerId)
      : m_clientSocket(clientSocket), m_id(playerId) {}

  int getClientSocket() const { return m_clientSocket; }
  int getId() const { return m_id; }
  Position getPosition() const { return m_position; }
  float getVelocityY() const { return m_velocityY; }
  bool isJetpacking() const { return m_isJetpacking; }
  int getScore() const { return m_Score; }
  PlayerState getState() const { return m_state; }

  void setPosition(float x, float y) { m_position = {x, y}; }
  void setVelocityY(float velocity) { m_velocityY = velocity; }
  void setJetpacking(bool jetpacking) { m_isJetpacking = jetpacking; }
  void setScore(int score) { m_Score = score; }
  void setState(PlayerState state) { m_state = state; }
};

enum class PacketType : uint8_t {
  CONNECT_REQUEST = 0x01,
  CONNECT_RESPONSE = 0x02,
  MAP_DATA = 0x03,
  GAME_START = 0x04,
  PLAYER_INPUT = 0x05,
  GAME_STATE_UPDATE = 0x06,
  PLAYER_POSITION = 0x07,
  COIN_COLLECTED = 0x08,
  PLAYER_DEATH = 0x09,
  GAME_OVER = 0x0A,
  PLAYER_DISCONNECT = 0x0B,
};

enum class GameState {
  WAITING_FOR_PLAYERS,
  IN_PROGRESS,
  GAME_OVER,
};

} // namespace Jetpack::Shared::Protocol
