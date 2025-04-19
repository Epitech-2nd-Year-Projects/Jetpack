#include "GameDisplay.hpp"

#include <iostream>
#include <cmath>

Jetpack::Client::GameDisplay::GameDisplay(int windowWidth, int windowHeight)
    : m_window(sf::VideoMode(windowWidth, windowHeight), "Jetpack") {
  loadResources();
  m_window.setFramerateLimit(60);

  m_topBoundary = 68.0f;
  m_bottomBoundary = 70.0f;
  m_backgroundHeight = 341.0f;
  m_playableHeight = m_backgroundHeight - m_topBoundary - m_bottomBoundary;
}

Jetpack::Client::GameDisplay::~GameDisplay() {
  if (m_window.isOpen()) {
    m_window.close();
  }
}

void Jetpack::Client::GameDisplay::loadResources() {
  if (!m_gameFont.loadFromFile("./resources/jetpack_font.ttf")) {
    std::cerr << "Failed to load jetpack_font.ttf!" << std::endl;
  }
  if (!m_backgroundTexture.loadFromFile("./resources/background.png")) {
    std::cerr << "Failed to load background.png!" << std::endl;
  }
  if (!m_playerSpritesheet.loadFromFile("./resources/player_sprite_sheet.png")) {
    std::cerr << "Failed to load player_sprite_sheet.png!" << std::endl;
  }
  if (!m_coinSpritesheet.loadFromFile("./resources/coins_sprite_sheet.png")) {
    std::cerr << "Failed to load coins_sprite_sheet.png!" << std::endl;
  }
  if (!m_zapperSpritesheet.loadFromFile("./resources/zapper_sprite_sheet.png")) {
    std::cerr << "Failed to load zapper_sprite_sheet.png!" << std::endl;
  }

  initializeParallaxBackgrounds();
  loadSounds();
  initializeAnimations();
}

void Jetpack::Client::GameDisplay::initializeParallaxBackgrounds() {
  if (m_backgroundTexture.getSize().x == 0) {
    std::cerr << "Background texture is invalid!" << std::endl;
    return;
  }

  
  m_parallaxLayers.clear();
  m_parallaxSpeeds.clear();

  
  const std::vector<float> speeds = {0.2f, 0.4f, 0.6f, 0.8f};

  
  const float cameraZoom = 2.0f;

  
  if (m_map.width > 0) {
    m_visibleMapWidth = m_map.width / cameraZoom;
  } else {
    
    m_visibleMapWidth = 10.0f; 
  }

  
  for (size_t i = 0; i < speeds.size(); i++) {
    sf::Sprite layer(m_backgroundTexture);

    
    float baseScale = static_cast<float>(m_window.getSize().y) / m_backgroundTexture.getSize().y;

    
    baseScale *= 1.2f;

    if (i == 0) {
      
      layer.setScale(baseScale * 0.95f, baseScale * 0.95f);
      layer.setColor(sf::Color(100, 100, 180, 150));
    } else if (i == 1) {
      layer.setScale(baseScale * 0.97f, baseScale * 0.97f);
      layer.setColor(sf::Color(150, 150, 200, 180));
    } else if (i == 2) {
      layer.setScale(baseScale * 0.99f, baseScale * 0.99f);
      layer.setColor(sf::Color(200, 200, 230, 210));
    } else {
      
      layer.setScale(baseScale, baseScale);
      layer.setColor(sf::Color(255, 255, 255, 255));
    }

    m_parallaxLayers.push_back(layer);
    m_parallaxSpeeds.push_back(speeds[i]);
  }
}


void Jetpack::Client::GameDisplay::updateParallaxBackgrounds(float deltaTime) {
  
  const float cameraZoom = 2.0f; 
  const float cameraOffsetX = 0.3f; 

  
  float playerX = 0.0f;
  std::lock_guard<std::mutex> lock(m_dataMutex);

  for (const auto &player : m_players) {
    if (player.getId() == m_localPlayerId) {
      playerX = player.getPosition().x;
      break;
    }
  }

  
  if (m_map.width > 0) {
    
    m_visibleMapWidth = m_map.width / cameraZoom;

    
    float targetCameraX = playerX - (m_visibleMapWidth * cameraOffsetX);

    
    targetCameraX = std::max(0.0f, targetCameraX);
    targetCameraX = std::min(targetCameraX, static_cast<float>(m_map.width - m_visibleMapWidth));

    
    const float cameraLerpFactor = 5.0f * deltaTime;
    m_cameraPositionX = m_cameraPositionX + (targetCameraX - m_cameraPositionX) * cameraLerpFactor;
  } else {
    
    m_backgroundScrollPosition += 50.0f * deltaTime;
    m_cameraPositionX = m_backgroundScrollPosition;
  }
}

void Jetpack::Client::GameDisplay::loadSounds() {
  if (!m_coinPickupBuffer.loadFromFile("./resources/coin_pickup_1.wav")) {
    std::cerr << "Failed to load coin_pickup_1.wav!" << std::endl;
  }
  if (!m_jetpackStartBuffer.loadFromFile("./resources/jetpack_start.wav")) {
    std::cerr << "Failed to load jetpack_start.wav!" << std::endl;
  }
  if (!m_jetpackLoopBuffer.loadFromFile("./resources/jetpack_lp.wav")) {
    std::cerr << "Failed to load jetpack_lp.wav!" << std::endl;
  }
  if (!m_jetpackStopBuffer.loadFromFile("./resources/jetpack_stop.wav")) {
    std::cerr << "Failed to load jetpack_stop.wav!" << std::endl;
  }
  if (!m_zapperBuffer.loadFromFile("./resources/dud_zapper_pop.wav")) {
    std::cerr << "Failed to load dud_zapper_pop.wav!" << std::endl;
  }

  m_coinPickupSound.setBuffer(m_coinPickupBuffer);
  m_jetpackStartSound.setBuffer(m_jetpackStartBuffer);
  m_jetpackLoopSound.setBuffer(m_jetpackLoopBuffer);
  m_jetpackLoopSound.setLoop(true);
  m_jetpackStopSound.setBuffer(m_jetpackStopBuffer);
  m_zapperSound.setBuffer(m_zapperBuffer);

  if (!m_gameMusic.openFromFile("./resources/theme.ogg")) {
    std::cerr << "Failed to load theme.ogg!" << std::endl;
  }
  m_gameMusic.setLoop(true);
  m_gameMusic.setVolume(50.0f);
  m_gameMusic.play();
}

void Jetpack::Client::GameDisplay::initializeAnimations() {
  const int playerFrameWidth = 134;
  const int playerFrameHeight = 134;
  const int numPlayerRunFrames = 4;

  for (int i = 0; i < numPlayerRunFrames; i++) {
    sf::IntRect frame(i * playerFrameWidth, 0, playerFrameWidth, playerFrameHeight);
    m_playerRunFrames.push_back(frame);
  }

  const int numPlayerJetpackFrames = 4;
  for (int i = 0; i < numPlayerJetpackFrames; i++) {
    sf::IntRect frame(i * playerFrameWidth, playerFrameHeight, playerFrameWidth, playerFrameHeight);
    m_playerJetpackFrames.push_back(frame);
  }

  const int coinFrameWidth = 192;
  const int coinFrameHeight = 171;
  const int numCoinFrames = 6;
  for (int i = 0; i < numCoinFrames; i++) {
    sf::IntRect frame(i * coinFrameWidth, 0, coinFrameWidth, coinFrameHeight);
    m_coinFrames.push_back(frame);
  }

  const int zapperFrameWidth = 47;
  const int zapperFrameHeight = 122;
  const int numZapperFrames = 4;
  for (int i = 0; i < numZapperFrames; i++) {
    sf::IntRect frame(i * zapperFrameWidth, 0, zapperFrameWidth, zapperFrameHeight);
    m_zapperFrames.push_back(frame);
  }
}

void Jetpack::Client::GameDisplay::run() {
  m_animationClock.restart();
  sf::Clock deltaClock;

  while (m_window.isOpen()) {
    float deltaTime = deltaClock.restart().asSeconds();

    processEvents();
    updateAnimations();
    updateParallaxBackgrounds(deltaTime);
    render();
  }
}

void Jetpack::Client::GameDisplay::updateAnimations() {
  float elapsedTime = m_animationClock.getElapsedTime().asSeconds();

  m_playerAnimFrame = static_cast<int>((elapsedTime * 10.0f)) % m_playerRunFrames.size();
  m_jetpackAnimFrame = static_cast<int>((elapsedTime * 15.0f)) % m_playerJetpackFrames.size();
  m_coinAnimFrame = static_cast<int>((elapsedTime * 8.0f)) % m_coinFrames.size();
  m_zapperAnimFrame = static_cast<int>((elapsedTime * 12.0f)) % m_zapperFrames.size();
  handleJetpackSounds();
}

void Jetpack::Client::GameDisplay::handleJetpackSounds() {
  std::lock_guard<std::mutex> lock(m_dataMutex);

  bool anyPlayerJetpacking = false;

  for (const auto &player : m_players) {
    if (player.getId() == m_localPlayerId && player.isJetpacking()) {
      anyPlayerJetpacking = true;
      break;
    }
  }

  if (anyPlayerJetpacking && !m_wasJetpacking) {
    m_jetpackStartSound.play();
    m_jetpackLoopSound.play();
    m_wasJetpacking = true;
  } else if (!anyPlayerJetpacking && m_wasJetpacking) {
    m_jetpackLoopSound.stop();
    m_jetpackStopSound.play();
    m_wasJetpacking = false;
  }
}

void Jetpack::Client::GameDisplay::processEvents() {
  sf::Event event;
  while (m_window.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      m_window.close();
    }
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::Space) {
      m_jetpackActive = true;
    } else if (event.type == sf::Event::KeyReleased &&
               event.key.code == sf::Keyboard::Space) {
      m_jetpackActive = false;
    }
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
      m_jetpackActive = true;
    } else if (event.type == sf::Event::MouseButtonReleased &&
               event.mouseButton.button == sf::Mouse::Left) {
      m_jetpackActive = false;
    }
  }
}

void Jetpack::Client::GameDisplay::render() {
  m_window.clear(sf::Color(10, 10, 30));

  std::lock_guard<std::mutex> lock(m_dataMutex);

  if (m_gameOver) {
    drawGameOver();
  } else {
    drawParallaxBackgrounds();
    drawMap();
    drawPlayers();
    drawUI();
  }
  m_window.display();
}

void Jetpack::Client::GameDisplay::drawParallaxBackgrounds() {
  
  sf::RectangleShape darkBackground(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  darkBackground.setFillColor(sf::Color(20, 20, 50));
  m_window.draw(darkBackground);

  
  for (size_t i = 0; i < m_parallaxLayers.size(); i++) {
    
    float parallaxOffset = m_cameraPositionX * m_parallaxSpeeds[i];

    
    float spriteWidth = m_backgroundTexture.getSize().x * m_parallaxLayers[i].getScale().x;
    int repetitions = static_cast<int>(std::ceil(m_window.getSize().x / spriteWidth)) + 2;

    
    float startX = -fmodf(parallaxOffset, spriteWidth);

    
    float verticalPosition = (m_window.getSize().y - m_backgroundTexture.getSize().y * m_parallaxLayers[i].getScale().y) / 2.0f;

    
    for (int j = -1; j < repetitions; j++) {
      float posX = startX + j * spriteWidth;
      m_parallaxLayers[i].setPosition(posX, verticalPosition);

      
      if (posX < m_window.getSize().x && posX + spriteWidth > 0) {
        m_window.draw(m_parallaxLayers[i]);
      }
    }
  }

  
  sf::RectangleShape gradientOverlay(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  sf::Color topColor(0, 0, 50, 30);
  sf::Color bottomColor(0, 0, 30, 50);
  gradientOverlay.setFillColor(bottomColor);
  m_window.draw(gradientOverlay);
}

void Jetpack::Client::GameDisplay::drawBackground() {
  sf::Sprite background(m_backgroundTexture);

  float scaleX = static_cast<float>(m_window.getSize().x) / m_backgroundTexture.getSize().x;
  float scaleY = static_cast<float>(m_window.getSize().y) / m_backgroundTexture.getSize().y;

  background.setScale(scaleX, scaleY);
  m_window.draw(background);

  if (m_debugMode) {
    sf::RectangleShape topLine(sf::Vector2f(m_window.getSize().x, 2.0f));
    topLine.setPosition(0, m_topBoundary * (static_cast<float>(m_window.getSize().y) / m_backgroundHeight));
    topLine.setFillColor(sf::Color::Red);
    m_window.draw(topLine);

    sf::RectangleShape bottomLine(sf::Vector2f(m_window.getSize().x, 2.0f));
    bottomLine.setPosition(0, m_window.getSize().y - (m_bottomBoundary * (static_cast<float>(m_window.getSize().y) / m_backgroundHeight)));
    bottomLine.setFillColor(sf::Color::Red);
    m_window.draw(bottomLine);
  }
}

void Jetpack::Client::GameDisplay::drawPlayers() {
  if (m_map.width == 0 || m_map.height == 0) {
    return;
  }

  
  const float cameraZoom = 2.0f;

  
  float visibleMapWidth = m_map.width / cameraZoom;

  
  float cellWidth = static_cast<float>(m_window.getSize().x) / visibleMapWidth;

  float windowHeight = static_cast<float>(m_window.getSize().y);
  float topOffset = m_topBoundary * (windowHeight / m_backgroundHeight);
  float bottomOffset = m_bottomBoundary * (windowHeight / m_backgroundHeight);
  float playableHeight = windowHeight - topOffset - bottomOffset;

  float cellHeight = playableHeight / m_map.height;

  for (const auto &player : m_players) {
    
    float screenX = (player.getPosition().x - m_cameraPositionX) * cellWidth;

    
    if (screenX < -cellWidth || screenX > m_window.getSize().x + cellWidth) {
      continue;
    }

    sf::Sprite playerSprite(m_playerSpritesheet);

    if (player.isJetpacking()) {
      playerSprite.setTextureRect(m_playerJetpackFrames[m_jetpackAnimFrame]);
    } else {
      playerSprite.setTextureRect(m_playerRunFrames[m_playerAnimFrame]);
    }

    float scale = 0.4f;
    playerSprite.setScale(scale, scale);

    float spriteWidth = playerSprite.getTextureRect().width * scale;
    float spriteHeight = playerSprite.getTextureRect().height * scale;
    float xPos = screenX + (cellWidth - spriteWidth) / 2;

    float yOffset = 10.0f;
    float relativePos = player.getPosition().y / m_map.height;
    float yPos = topOffset + (relativePos * playableHeight) + (cellHeight - spriteHeight) / 2 - yOffset;

    yPos = std::max(yPos, topOffset);
    yPos = std::min(yPos, windowHeight - bottomOffset - spriteHeight);

    playerSprite.setPosition(xPos, yPos);

    if (player.getId() == m_localPlayerId) {
      playerSprite.setColor(sf::Color(200, 255, 200));
    } else {
      playerSprite.setColor(sf::Color(255, 200, 200));
    }

    if (m_debugMode) {
      sf::RectangleShape hitbox;
      hitbox.setSize(sf::Vector2f(cellWidth * 0.8f, cellHeight * 0.8f));
      hitbox.setPosition(screenX + cellWidth * 0.1f,
                         topOffset + (player.getPosition().y / m_map.height) * playableHeight + cellHeight * 0.1f);
      hitbox.setFillColor(sf::Color(0, 0, 0, 0));
      hitbox.setOutlineColor(sf::Color::Red);
      hitbox.setOutlineThickness(1.0f);
      m_window.draw(hitbox);
    }
    m_window.draw(playerSprite);
  }
}

void Jetpack::Client::GameDisplay::drawMap() {
  if (m_map.width == 0 || m_map.height == 0) {
    return;
  }

  const float cameraZoom = 2.0f;
  float visibleMapWidth = m_map.width / cameraZoom;
  float cellWidth = static_cast<float>(m_window.getSize().x) / visibleMapWidth;
  float windowHeight = static_cast<float>(m_window.getSize().y);
  float topOffset = m_topBoundary * (windowHeight / m_backgroundHeight);
  float bottomOffset = m_bottomBoundary * (windowHeight / m_backgroundHeight);
  float playableHeight = windowHeight - topOffset - bottomOffset;

  float cellHeight = playableHeight / m_map.height;
  
  int startCol = static_cast<int>(m_cameraPositionX);
  startCol = std::max(0, startCol);
  
  int endCol = static_cast<int>(m_cameraPositionX + visibleMapWidth + 1);
  endCol = std::min(endCol, m_map.width);

  for (int i = 0; i < m_map.height; i++) {
    for (int j = startCol; j < endCol; j++) {
      
      float xPos = (j - m_cameraPositionX) * cellWidth;
      float yPos = topOffset + i * cellHeight;

      if (m_debugMode && m_map.tiles[i][j] != Shared::Protocol::TileType::EMPTY) {
        sf::RectangleShape cellHitbox;
        cellHitbox.setSize(sf::Vector2f(cellWidth * 0.8f, cellHeight * 0.8f));
        cellHitbox.setPosition(xPos + cellWidth * 0.1f, yPos + cellHeight * 0.1f);
        cellHitbox.setFillColor(sf::Color(0, 0, 0, 0));
        cellHitbox.setOutlineColor(sf::Color::Yellow);
        cellHitbox.setOutlineThickness(1.0f);
        m_window.draw(cellHitbox);
      }

      switch (m_map.tiles[i][j]) {
      case Shared::Protocol::TileType::COIN:
        {
          sf::Sprite coinSprite(m_coinSpritesheet);
          coinSprite.setTextureRect(m_coinFrames[m_coinAnimFrame]);

          float scale = 0.2f;
          coinSprite.setScale(scale, scale);

          float spriteWidth = m_coinFrames[0].width * scale;
          float spriteHeight = m_coinFrames[0].height * scale;

          coinSprite.setPosition(xPos + (cellWidth - spriteWidth) / 2,
                                yPos + (cellHeight - spriteHeight) / 2);

          m_window.draw(coinSprite);
        }
        break;
      case Shared::Protocol::TileType::ELECTRICSQUARE:
        {
          sf::Sprite zapperSprite(m_zapperSpritesheet);
          zapperSprite.setTextureRect(m_zapperFrames[m_zapperAnimFrame]);

          float scale = 0.6f;
          zapperSprite.setScale(scale, scale);

          float spriteWidth = m_zapperFrames[0].width * scale;
          float spriteHeight = m_zapperFrames[0].height * scale;

          zapperSprite.setPosition(xPos + (cellWidth - spriteWidth) / 2,
                                  yPos + (cellHeight - spriteHeight) / 2);
          m_window.draw(zapperSprite);
        }
        break;
      default:
        break;
      }
    }
  }
}

void Jetpack::Client::GameDisplay::drawUI() {
  if (m_players.empty()) {
    sf::Text waitingText;
    waitingText.setFont(m_gameFont);
    waitingText.setCharacterSize(30);
    waitingText.setString("Waiting for other players...");
    waitingText.setFillColor(sf::Color::White);

    sf::FloatRect textRect = waitingText.getLocalBounds();
    waitingText.setOrigin(textRect.width / 2.0f, textRect.height / 2.0f);
    waitingText.setPosition(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);

    m_window.draw(waitingText);
    return;
  }

  int yOffset = 10;
  for (const auto &player : m_players) {
    sf::Text scoreText;
    scoreText.setFont(m_gameFont);
    scoreText.setCharacterSize(20);
    scoreText.setOutlineThickness(2.0f);
    scoreText.setOutlineColor(sf::Color::Black);

    if (player.getId() == m_localPlayerId) {
      scoreText.setString("You: " + std::to_string(player.getScore()));
      scoreText.setFillColor(sf::Color::Green);
    } else {
      scoreText.setString("Player " + std::to_string(player.getId()) + ": " +
                          std::to_string(player.getScore()));
      scoreText.setFillColor(sf::Color::Red);
    }

    scoreText.setPosition(10, yOffset);
    m_window.draw(scoreText);
    yOffset += 30;
  }
}

void Jetpack::Client::GameDisplay::drawGameOver() {
  sf::RectangleShape overlay(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
  overlay.setFillColor(sf::Color(0, 0, 0, 200));
  m_window.draw(overlay);

  sf::Text gameOverText;
  gameOverText.setFont(m_gameFont);
  gameOverText.setCharacterSize(60);
  gameOverText.setString("GAME OVER");
  gameOverText.setFillColor(sf::Color::White);
  gameOverText.setOutlineThickness(3.0f);
  gameOverText.setOutlineColor(sf::Color::Black);

  sf::FloatRect textRect = gameOverText.getLocalBounds();
  gameOverText.setOrigin(textRect.left + textRect.width / 2.0f,
                       textRect.top + textRect.height / 2.0f);
  gameOverText.setPosition(m_window.getSize().x / 2.0f,
                         m_window.getSize().y / 2.0f - 50);

  m_window.draw(gameOverText);

  sf::Text resultText;
  resultText.setFont(m_gameFont);
  resultText.setCharacterSize(40);
  resultText.setOutlineThickness(2.0f);
  resultText.setOutlineColor(sf::Color::Black);

  if (m_winnerId == m_localPlayerId) {
    resultText.setString("You win!");
    resultText.setFillColor(sf::Color::Green);
  } else if (m_winnerId > 0) {
    resultText.setString("Player " + std::to_string(m_winnerId) + " Wins!");
    resultText.setFillColor(sf::Color::Red);
  } else {
    resultText.setString("No winner");
    resultText.setFillColor(sf::Color::Yellow);
  }

  textRect = resultText.getLocalBounds();
  resultText.setOrigin(textRect.left + textRect.width / 2.0f,
                     textRect.top + textRect.height / 2.0f);
  resultText.setPosition(m_window.getSize().x / 2.0f,
                       m_window.getSize().y / 2.0f + 50);
  m_window.draw(resultText);
}

void Jetpack::Client::GameDisplay::updateMap(
    const Shared::Protocol::GameMap &map) {
  std::lock_guard<std::mutex> lock(m_dataMutex);
  m_map = map;
}

void Jetpack::Client::GameDisplay::updateGameState(
    const std::vector<Shared::Protocol::Player> &players) {
  std::lock_guard<std::mutex> lock(m_dataMutex);
  std::cout << "Updating game state with " << players.size() << " players"
            << std::endl;
  m_players = players;
}

void Jetpack::Client::GameDisplay::handleCoinCollected(int playerId, int x,
                                                       int y) {
  std::lock_guard<std::mutex> lock(m_dataMutex);

  if (x >= 0 && x < m_map.width && y >= 0 && y < m_map.height) {
    m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
  }

  for (auto &player : m_players) {
    if (player.getId() == playerId) {
      player.setScore(player.getScore() + 1);

      if (playerId == m_localPlayerId) {
        m_coinPickupSound.play();
      }
      break;
    }
  }
}

void Jetpack::Client::GameDisplay::handlePlayerDeath(int playerId) {
  std::lock_guard<std::mutex> lock(m_dataMutex);

  for (auto &player : m_players) {
    if (player.getId() == playerId) {
      player.setState(Shared::Protocol::PlayerState::DEAD);

      if (playerId == m_localPlayerId) {
        m_zapperSound.play();
      }
      break;
    }
  }
}

void Jetpack::Client::GameDisplay::handleGameOver(int winnerId) {
  std::lock_guard<std::mutex> lock(m_dataMutex);

  m_gameOver = true;
  m_winnerId = winnerId;
  m_jetpackLoopSound.stop();
}

bool Jetpack::Client::GameDisplay::isJetpackActive() const {
  return m_jetpackActive;
}

void Jetpack::Client::GameDisplay::setLocalPlayerId(int id) {
  m_localPlayerId = id;
}

void Jetpack::Client::GameDisplay::setDebugMode(bool debug) {
  m_debugMode = debug;
}