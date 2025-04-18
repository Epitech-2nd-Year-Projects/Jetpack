#include "NetworkClient.hpp"
#include "../Shared/Exceptions.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

bool Jetpack::Client::NetworkClient::connectToServer() {
  m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_serverSocket < 0) {
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to create socket");
  }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(m_serverPort);

  if (inet_pton(AF_INET, m_serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
    ::close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Invalid server address");
  }

  if (::connect(m_serverSocket, (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0) {
    ::close(m_serverSocket);
    throw Jetpack::Shared::Exceptions::SocketException(
        "Failed to connect to server");
  }

  int flags = fcntl(m_serverSocket, F_GETFL, 0);
  fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

  uint8_t buffer[2];
  buffer[0] =
      static_cast<uint8_t>(Shared::Protocol::PacketType::CONNECT_REQUEST);
  buffer[1] = 0;

  if (::send(m_serverSocket, buffer, sizeof(buffer), 0) != sizeof(buffer)) {
    std::cerr << "Failed to send connection request" << std::endl;
    close(m_serverSocket);
    m_serverSocket = -1;
    return false;
  }

  return true;
}
