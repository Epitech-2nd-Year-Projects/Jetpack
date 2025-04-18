#pragma once

#include "../Shared/Protocol.hpp"
#include <SFML/Graphics.hpp>
#include <mutex>

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

        bool isJetpackActive() const;

    private:
        sf::RenderWindow m_window;

        sf::Texture m_backgroundTexture;
        sf::Texture m_playerTexture;
        sf::Texture m_coinTexture;
        sf::Texture m_electricTexture;

        sf::Font m_font;

        std::mutex m_dataMutex;
        Shared::Protocol::GameMap m_map;
        std::vector<Shared::Protocol::Player> m_players;
        int m_localPlayerId = 1;
        bool m_gameOver = false;
        int m_winnerId = -1;

        bool m_jetpackActive = false;

        void render();
        void drawBackground();
        void drawMap();
        void drawPlayers();
        void drawUI();
        void drawGameOver();

        void processEvents();

        void loadResources();
    };
}