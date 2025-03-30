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

class ScopedFdException : public Exception {
public:
  explicit ScopedFdException(std::string message)
      : Exception(std::move(message)) {}
};
} // namespace Jetpack::Shared::Exceptions
