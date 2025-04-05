#include "Server.hpp"
#include "../Shared/Exceptions.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/fcntl.h>

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
