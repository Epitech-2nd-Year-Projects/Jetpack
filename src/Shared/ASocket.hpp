#pragma once

#include "ISocket.hpp"
#include "ScopedFd.hpp"

namespace Jetpack::Shared {
class ASocket : public ISocket {
public:
  int Receive(char *buffer, int size) override;
  int Send(const char *data, int size) override;

  int GetFd() const { return m_fd.Get(); }

protected:
  ScopedFd m_fd;
};
} // namespace Jetpack::Shared
