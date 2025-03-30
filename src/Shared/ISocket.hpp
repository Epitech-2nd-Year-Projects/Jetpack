#pragma once

namespace Jetpack::Shared {
class ISocket {
public:
  virtual ~ISocket() = default;

  virtual int Send(const char *data, int size) = 0;
  virtual int Receive(char *buffer, int size) = 0;
};
} // namespace Jetpack::Shared
