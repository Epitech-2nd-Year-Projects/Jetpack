#pragma once

#include "Exceptions.hpp"
#include <unistd.h>

namespace Jetpack::Shared {
class ScopedFd {
public:
  ScopedFd() = default;
  explicit ScopedFd(int fd) : m_fd(fd) {
    if (fd < 0) {
      throw Exceptions::ScopedFdException("Invalid file descriptor: " +
                                          std::to_string(fd));
    }
  }
  ~ScopedFd() {
    if (m_fd >= 0) {
      ::close(m_fd);
    }
  }

  ScopedFd(const ScopedFd &) = delete;
  ScopedFd &operator=(const ScopedFd &) = delete;

  ScopedFd(ScopedFd &&other) noexcept : m_fd(other.m_fd) { other.m_fd = -1; }
  ScopedFd &operator=(ScopedFd &&other) noexcept {
    if (this != &other) {
      if (m_fd >= 0) {
        ::close(m_fd);
      }
      m_fd = other.m_fd;
      other.m_fd = -1;
    }
    return *this;
  }

  int Get() const noexcept { return m_fd; }

private:
  int m_fd = -1;
};
}; // namespace Jetpack::Shared
