#pragma once

#include "../Shared/Protocol.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <mutex>
#include <vector>

namespace Jetpack::Client {

class GameDisplay {
public:
  GameDisplay(int windowWidth = 1920, int windowHeight = 1080);
  ~GameDisplay();

  void run();

  void updateMap(const Shared::Protocol::GameMap &map);
  void updateGameState(const std::vector<Shared::Protocol::Player> &players);
  void handleCoinCollected(int playerId, int x, int y);
  void handlePlayerDeath(int playerId);
  void handleGameOver(int winnerId);

  void setLocalPlayerId(int id);
  void setDebugMode(bool debug);

  bool isJetpackActive() const;

private:
  sf::RenderWindow m_window;

  sf::Texture m_backgroundTexture;
  sf::Texture m_playerSpritesheet;
  sf::Texture m_coinSpritesheet;
  sf::Texture m_zapperSpritesheet;

  sf::Font m_gameFont;

  std::vector<sf::IntRect> m_playerRunFrames;
  std::vector<sf::IntRect> m_playerJetpackFrames;
  std::vector<sf::IntRect> m_coinFrames;
  std::vector<sf::IntRect> m_zapperFrames;

  sf::Clock m_animationClock;
  int m_playerAnimFrame = 0;
  int m_jetpackAnimFrame = 0;
  int m_coinAnimFrame = 0;
  int m_zapperAnimFrame = 0;

  sf::SoundBuffer m_coinPickupBuffer;
  sf::SoundBuffer m_jetpackStartBuffer;
  sf::SoundBuffer m_jetpackLoopBuffer;
  sf::SoundBuffer m_jetpackStopBuffer;
  sf::SoundBuffer m_zapperBuffer;

  sf::Sound m_coinPickupSound;
  sf::Sound m_jetpackStartSound;
  sf::Sound m_jetpackLoopSound;
  sf::Sound m_jetpackStopSound;
  sf::Sound m_zapperSound;

  sf::Music m_gameMusic;

  bool m_wasJetpacking = false;
  bool m_debugMode = false;

  std::mutex m_dataMutex;
  Shared::Protocol::GameMap m_map;
  std::vector<Shared::Protocol::Player> m_players;
  int m_localPlayerId = 1;
  bool m_gameOver = false;
  int m_winnerId = -1;

  bool m_jetpackActive = false;

  // Playable area boundaries
  float m_topBoundary = 0.0f;
  float m_bottomBoundary = 0.0f;
  float m_backgroundHeight = 0.0f;
  float m_playableHeight = 0.0f;

  void render();
  void drawBackground();
  void drawMap();
  void drawPlayers();
  void drawUI();
  void drawGameOver();

  void processEvents();
  void updateAnimations();
  void handleJetpackSounds();

  void loadResources();
  void loadSounds();
  void initializeAnimations();
};
} // namespace Jetpack::Client