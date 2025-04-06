#include "Server.hpp"
#include "../Shared/Exceptions.hpp"
#include "../Shared/Protocol.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <vector>

void Jetpack::Server::GameServer::start() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to create socket: " + std::string(strerror(errno)));
  }

  int opt = 1;
  if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to set socket options: " + std::string(strerror(errno)));
  }

  fcntl(m_serverSocket, F_SETFL, O_NONBLOCK);

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(m_port);

  if (bind(m_serverSocket, reinterpret_cast<sockaddr *>(&serverAddr),
           sizeof(serverAddr)) < 0) {
    close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to bind socket: " + std::string(strerror(errno)));
  }

  if (listen(m_serverSocket, 10) < 0) {
    close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to listen on socket: " + std::string(strerror(errno)));
  }

  m_running = true;
  m_gameStarted = false;
}

void Jetpack::Server::GameServer::stop() {
  m_running = false;

  if (m_serverSocket >= 0) {
    close(m_serverSocket);
    m_serverSocket = -1;
  }
}

void Jetpack::Server::GameServer::loadMap() {
  std::ifstream file(m_mapFile);
  if (!file.is_open()) {
    throw Jetpack::Shared::Exceptions::MapLoaderException(
        "Failed to open map file: " + m_mapFile.string());
  }

  std::string line;
  while (std::getline(file, line)) {
    std::vector<Jetpack::Shared::Protocol::TileType> row;
    for (char c : line) {
      switch (c) {
      case ' ':
        row.push_back(Jetpack::Shared::Protocol::TileType::Empty);
        break;
      case '#':
        row.push_back(Jetpack::Shared::Protocol::TileType::Wall);
        break;
      case 'C':
        row.push_back(Jetpack::Shared::Protocol::TileType::Coin);
        m_map.coinPositions.push_back({static_cast<float>(row.size() - 1),
                                       static_cast<float>(m_map.tiles.size())});
        break;
      case 'X':
        row.push_back(Jetpack::Shared::Protocol::TileType::ElectricSquare);
        break;
      case 'E':
        row.push_back(Jetpack::Shared::Protocol::TileType::EndPoint);
        break;
      default:
        row.push_back(Jetpack::Shared::Protocol::TileType::Empty);
      }
    }
    if (!row.empty()) {
      m_map.tiles.push_back(row);
      if (row.size() > m_map.width) {
        m_map.width = row.size();
      }
    }
  }
  m_map.height = m_map.tiles.size();

  if (m_map.tiles.empty()) {
    throw Jetpack::Shared::Exceptions::MapLoaderException(
        "Map file is empty or invalid: " + m_mapFile.string());
  }
}
