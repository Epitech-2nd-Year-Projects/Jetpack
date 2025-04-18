#include "GameDisplay.hpp"

#include <bits/ranges_base.h>

Jetpack::Client::GameDisplay::GameDisplay(int windowWidth, int windowHeight)
    : m_window(sf::VideoMode(windowWidth, windowHeight), "Jetpack") {
    loadResources();
    m_window.setFramerateLimit(60);
}

Jetpack::Client::GameDisplay::~GameDisplay() {
    if (m_window.isOpen()) {
        m_window.close();
    }
}

void Jetpack::Client::GameDisplay::loadResources() {
    m_backgroundTexture.create(100, 100);
    sf::Image backgroundImage;
    backgroundImage.create(100, 100, sf::Color(20, 20, 50));
    m_backgroundTexture.update(backgroundImage);

    backgroundImage.loadFromFile()
}

void Jetpack::Client::GameDisplay::run() {
    while (m_window.isOpen()) {
        processEvents();
        render();
    }
}

void Jetpack::Client::GameDisplay::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
        m_jetpackActive = true;
    } else if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Space) {
        m_jetpackActive = false;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        m_jetpackActive = true;
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_jetpackActive = false;
    }
}

void Jetpack::Client::GameDisplay::render() {
    m_window.clear(sf::Color(10, 10, 30));

    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_gameOver) {
        drawGameOver();
    } else {
        drawBackground();
        drawMap();
        drawPlayers();
        drawUI();
    }
    m_window.display();
}

void Jetpack::Client::GameDisplay::drawBackground() {
    sf::Sprite background(m_backgroundTexture);
    background.setScale(
        static_cast<float>(m_window.getSize().x) / m_backgroundTexture.getSize().x,
        static_cast<float>(m_window.getSize().y) / m_backgroundTexture.getSize().y
        );
    m_window.draw(background);
}

void Jetpack::Client::GameDisplay::drawMap() {
    if (m_map.width == 0 || m_map.height == 0) {
        return;
    }

    float cellWidth = static_cast<float>(m_window.getSize().x) / m_map.width;
    float cellHeight = static_cast<float>(m_window.getSize().y) / m_map.height;

    for (int i = 0; i < m_map.height; i++) {
        for (int j = 0; j < m_map.width; j++) {
            sf::RectangleShape cell(sf::Vector2f(cellWidth,  cellHeight));
            cell.setPosition(x * cellWidth, y * cellHeight);

            switch (m_map.tiles[i][j]) {
                case Shared::Protocol::TileType::COIN:
                    cell.setTexture(&m_coinTexture);
                    cell.setFillColor(sf::Color::Yellow);
                    m_window.draw(cell);
                    break;
                case Shared::Protocol::TileType::ELECTRICSQUARE:
                    cell.setTexture(&m_electricTexture);
                    cell.setFillColor(sf::Color::Cyan);
                    m_window.draw(cell);
                    break;
                default:
                    break;
            }
        }
    }
}

void Jetpack::Client::GameDisplay::drawPlayers() {
    if (m_map.width == 0 || m_map.height == 0) {
        return;
    }

    float cellWidth = static_cast<float>(m_window.getSize().x) / m_map.width;
    float cellHeight = static_cast<float>(m_window.getSize().y) / m_map.height;

    for (const auto &player : m_players) {
        sf::RectangleShape playerShape(sf::Vector2f(cellWidth * 0.8f, cellHeight * 0.8f));
        playerShape.setPosition(
            player.getPosition().x * cellWidth + cellWidth * 0.1f,
            player.getPosition().y * cellHeight + cellHeight * 0.1f
        );

        if (player.getId() == m_localPlayerId) {
            playerShape.setFillColor(sf::Color::Green);
        } else {
            playerShape.setFillColor(sf::Color::Red);
        }

        if (player.isJetpacking()) {
            sf::RectangleShape jetpackFlame(sf::Vector2f(cellWidth * 0.4f, cellHeight * 0.6f));
            jetpackFlame.setPosition(
                player.getPosition().x * cellWidth + cellWidth * 0.3f,
                player.getPosition().y + cellHeight + cellHeight * 1.0f
            );
            jetpackFlame.setFillColor(sf::Color(255, 150, 0));
            m_window.draw(jetpackFlame);
        }
        m_window.draw(playerShape);
    }
}

void Jetpack::Client::GameDisplay::drawUI() {
    int yOffset = 10;
    for (const auto &player : m_players) {
        sf::Text scoreText;
        scoreText.setFont(m_font);
        scoreText.setCharacterSize(20);

        if (player.getId() == m_localPlayerId) {
            scoreText.setString("You: " + std::to_string(player.getScore()));
            scoreText.setFillColor(sf::Color::Green);
        } else {
            scoreText.setString("Player " + std::to_string(player.getId()) + ": " + std::to_string(player.getScore()));
            scoreText.setFillColor(sf::Color::Red);
        }

        scoreText.setPosition(10, yOffset);
        m_window.draw(scoreText);
        yOffset += 30;
    }
}


void Jetpack::Client::GameDisplay::drawGameOver() {
    sf::Text gameOverText;
    gameOverText.setFont(m_font);
    gameOverText.setCharacterSize(40);
    gameOverText.setString("GAME OVER");
    gameOverText.setFillColor(sf::Color::White);

    sf::FloatRect textRect = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    gameOverText.setPosition(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f - 50);

    m_window.draw(gameOverText);

    sf::Text resultText;
    resultText.setFont(m_font);
    resultText.setCharacterSize(30);

    if (m_winnerId == m_localPlayerId) {
        resultText.setString("You win !");
        resultText.setFillColor(sf::Color::Green);
    } else if (m_winnerId > 0) {
        resultText.setString("Player " + std::to_string(m_winnerId) + " Wins !");
        resultText.setFillColor(sf::Color::Green);
    } else {
        resultText.setString("No winner");
        resultText.setFillColor(sf::Color::Yellow);
    }

    textRect = resultText.getLocalBounds();
    resultText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    resultText.setPosition(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f + 50);
    m_window.draw(resultText);
}

void Jetpack::Client::GameDisplay::updateMap(const Shared::Protocol::GameMap &map) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_map = map;
}

void Jetpack::Client::GameDisplay::updateGameState(const std::vector<Shared::Protocol::Player> &players) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_players = players;
}

void Jetpack::Client::GameDisplay::handleCoinCollected(int playerId, int x, int y) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (x >= 0 && x < m_map.width && y >= 0 && y < m_map.height) {
        m_map.tiles[y][x] = Shared::Protocol::TileType::EMPTY;
    }

    for (auto &player : m_players) {
        if (player.getId() == playerId) {
            player.setScore(player.getScore() + 1);
            break;
        }
    }
}

void Jetpack::Client::GameDisplay::handlePlayerDeath(int playerId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    for (auto &player : m_players) {
        if (player.getId() == playerId) {
            player.setState(Shared::Protocol::PlayerState::DEAD);
            break;
        }
    }
}

void Jetpack::Client::GameDisplay::handleGameOver(int winnerId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    m_gameOver = true;
    m_winnerId = winnerId;
}

bool Jetpack::Client::GameDisplay::isJetpackActive() const {
    return m_jetpackActive;
}
