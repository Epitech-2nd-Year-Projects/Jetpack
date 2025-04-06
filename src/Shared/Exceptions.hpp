#pragma once

#include <exception>
#include <string>

namespace Jetpack::Shared::Exceptions {
class Exception : public std::exception {
public:
  explicit Exception(std::string message) : m_message(std::move(message)) {}

  [[nodiscard]] const char *what() const noexcept override {
    return m_message.c_str();
  }

private:
  std::string m_message;
};

class SocketException : public Exception {
public:
  explicit SocketException(const std::string &message)
      : Exception("Socket error: " + message) {}
};

class GameServerException : public Exception {
public:
  explicit GameServerException(const std::string &message)
      : Exception("Game server error: " + message) {}
};

class MapLoaderException : public GameServerException {
public:
  explicit MapLoaderException(const std::string &message)
      : GameServerException("Map loader error: " + message) {}
};
} // namespace Jetpack::Shared::Exceptions
