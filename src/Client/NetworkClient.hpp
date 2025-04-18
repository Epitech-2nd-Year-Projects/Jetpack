#pragma once

#include "../Shared/Protocol.hpp"
#include <string>
#include <unistd.h>
namespace Jetpack::Client {
class NetworkClient {
public:
  NetworkClient(int serverPort = 8080, const std::string serverAddress = "",
                bool debugMode = false)
      : m_serverPort(serverPort), m_serverAddress(serverAddress),
        m_debugMode(debugMode) {}
  ~NetworkClient() {
    if (m_serverSocket != -1) {
      ::close(m_serverSocket);
    }
  }

  bool connectToServer();

private:
  int m_serverPort;
  std::string m_serverAddress;
  bool m_debugMode = false;
  int m_serverSocket = -1;
};
} // namespace Jetpack::Client
