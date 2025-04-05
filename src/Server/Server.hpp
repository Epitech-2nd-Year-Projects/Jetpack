#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace Jetpack::Server {
class GameServer {
public:
  GameServer(uint16_t port, const std::string &mapFile)
      : m_port(port), m_mapFile(mapFile) {}
  ~GameServer();

  void start();
  void stop();

private:
  uint16_t m_port;

  std::filesystem::path m_mapFile;

  bool m_running = false;
  bool m_gameStarted = false;

  int m_serverSocket = -1;
};
} // namespace Jetpack::Server
